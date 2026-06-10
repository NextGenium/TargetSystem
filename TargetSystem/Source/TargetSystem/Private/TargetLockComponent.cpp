// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetLockComponent.h"
#include "TargetPointComponent.h"
#include "TargetSystemInterface.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "TargetSystemLog.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Targeting/TargetLockContext.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"

UTargetLockComponent::UTargetLockComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    // LockedOnWidgetClass is assigned project-side via the EditAnywhere UPROPERTY.
    // The plugin ships no Content, so no default widget is loaded here.
    TargetCollisionChannel = ECC_Pawn;
}

void UTargetLockComponent::SetUp(
    bool _bAdjustPitchBasedOnDistanceToTarget,
    bool _bAdjustPitchBasedOnDistanceToTargetUsingCurve
)
{
    bAdjustPitchBasedOnDistanceToTarget = _bAdjustPitchBasedOnDistanceToTarget;
    bAdjustPitchBasedOnDistanceToTargetUsingCurve = _bAdjustPitchBasedOnDistanceToTargetUsingCurve;
}

void UTargetLockComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		TS_LOG(Error, TEXT("[%s] TargetSystemComponent: Cannot get Owner reference ..."), *GetName());
		return;
	}

	OwnerPawn = Cast<APawn>(OwnerActor);
	if (!ensure(OwnerPawn))
	{
		TS_LOG(Error, TEXT("[%s] TargetSystemComponent: Component is meant to be added to Pawn only ..."), *GetName());
		return;
	}

	SetupLocalPlayerController();
}

void UTargetLockComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bTargetLocked || !NearestTarget) return;

    SetControlRotationOnTarget();
}

void UTargetLockComponent::StartObservingTarget()
{
    bTargetLocked = true;
    NearestTarget->StartTargetable();

    // Lock onto the target's first target point — drives the reticle widget attach and the
    // per-point pitch curve. Point-selection via the preset is a later follow-up.
    const TArray<UTargetPointComponent*> TargetPoints = NearestTarget->GetTargetPoints();
    LockedPoint = TargetPoints.IsEmpty() ? nullptr : TargetPoints[0];

    if (OnTargetLockedOn.IsBound())
    {
        OnTargetLockedOn.Broadcast(GetTargetOwnerActor(NearestTarget));
    }

    SetupLocalPlayerController();
    ControlRotation(true);

    if (IsValid(OwnerPlayerController))
    {
        OwnerPlayerController->SetIgnoreLookInput(true);
    }

    CreateAndAttachTargetLockedOnWidgetComponent(NearestTarget);

    GetWorld()->GetTimerManager().SetTimer(ObservingTimer, this, &UTargetLockComponent::UpdateTargetInfo, TimerTick, true);
}

void UTargetLockComponent::UpdateTargetInfo()
{
    FHitResult Hit;
    if(NearestTarget->IsTargetable() && !LineTrace(GetOwner()->GetActorLocation(), GetTargetOwnerLocation(NearestTarget), Hit))
    {
        if (BehindWallTimer.IsValid()) return;
        GetWorld()->GetTimerManager().SetTimer(BehindWallTimer, [this]() { StopObservingTarget(true); }, BreakLineOfSightDelay, false);
        return;
    }
    GetWorld()->GetTimerManager().ClearTimer(BehindWallTimer);

    if (NearestTarget
        && NearestTarget->IsTargetable()
        && GetDistanceFromTarget(NearestTarget) <= LoseTargetDistance) return;

    StopObservingTarget(false, true);
}

void UTargetLockComponent::StopObservingTarget(const bool bIgnoreAutoSwitch, const bool bTargetIsDead)
{
    if (NearestTarget)
    {
        if (OnTargetLockedOff.IsBound())
        {
            OnTargetLockedOff.Broadcast(GetTargetOwnerActor(NearestTarget));
        }

        NearestTarget->StopTargetable();
        if (bTargetIsDead)
        {
            PotentialTargets.Remove(NearestTarget);
            if (OnTargetIsDead.IsBound())
            {
                OnTargetIsDead.Broadcast(NearestTarget);
            }
        }
    }
    GetWorld()->GetTimerManager().ClearTimer(ObservingTimer);

    if (TargetLockedOnWidgetComponent)
    {
        TargetLockedOnWidgetComponent->DestroyComponent();
    }

    if (bIsSwitchingTarget) return;

    if (bAutoTargetSwitch && !bIgnoreAutoSwitch)
    {
        AutoSwitchTarget();
        return;
    }

    StopTargetLock();
}

void UTargetLockComponent::MessageFinishTargetLock() const
{
    if (OnFinishTargetLock.IsBound())
    {
        OnFinishTargetLock.Broadcast();
    }
}

AActor* UTargetLockComponent::ExtractTargetingResults(FTargetingRequestHandle Handle, TArray<TargetInterface>& OutTargets)
{
    OutTargets.Reset();

    UWorld* World = GetWorld();
    UTargetingSubsystem* Subsystem = World ? UTargetingSubsystem::Get(World) : nullptr;
    if (!Subsystem)
    {
        return nullptr;
    }

    TArray<AActor*> ResultActors;
    Subsystem->GetTargetingResultsActors(Handle, ResultActors);

    AActor* First = nullptr;
    for (AActor* Actor : ResultActors)
    {
        if (!IsValid(Actor)) continue;
        if (!First) First = Actor;
        OutTargets.Add(Actor);
    }

    return First;
}

void UTargetLockComponent::OnTargetingCompleted(FTargetingRequestHandle Handle)
{
    PotentialTargets.Reset();
    AActor* First = ExtractTargetingResults(Handle, PotentialTargets);
    if (!First)
    {
        MessageFinishTargetLock();
        return;
    }

    NearestTarget = First;
    StartObservingTarget();
}

void UTargetLockComponent::OnSwitchTargetingCompleted(FTargetingRequestHandle Handle)
{
    TArray<TargetInterface> Results;
    AActor* First = ExtractTargetingResults(Handle, Results);
    if (!First)
    {
        return;
    }

    bIsSwitchingTarget = true;
    StopObservingTarget();
    NearestTarget = First;
    StartObservingTarget();
    ResetIsSwitchingTarget();
}

void UTargetLockComponent::TryStartTargetLock()
{
    if (!IsValid(TargetingPreset))
    {
        MessageFinishTargetLock();
        return;
    }

    UWorld* World = GetWorld();
    UTargetingSubsystem* Subsystem = World ? UTargetingSubsystem::Get(World) : nullptr;
    if (!Subsystem)
    {
        MessageFinishTargetLock();
        return;
    }

    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = GetOwner();
    UTargetLockContext* TargetLockContext = NewObject<UTargetLockContext>(this);
    TargetLockContext->Mode = ETargetSwitchMode::LockOn;
    SourceContext.SourceObject = TargetLockContext;

    const FTargetingRequestHandle TargetingHandle =
        UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
    const FTargetingRequestDelegate Delegate = FTargetingRequestDelegate::CreateUObject(
        this, &UTargetLockComponent::OnTargetingCompleted);

    Subsystem->StartAsyncTargetingRequestWithHandle(TargetingHandle, Delegate);
}

void UTargetLockComponent::StopTargetLock()
{
    SetupLocalPlayerController();

    bTargetLocked = false;

    if (NearestTarget)
    {
        ControlRotation(false);

        if (IsValid(OwnerPlayerController))
        {
            OwnerPlayerController->ResetIgnoreLookInput();
        }
    }
    PotentialTargets.Empty();

    NearestTarget = nullptr;

    MessageFinishTargetLock();
}

void UTargetLockComponent::SwitchTarget(FVector2D AxisValue)
{
    if (!CanSwitchTarget(AxisValue)) return;
    if (bIsSwitchingTarget) return;
    if (!IsValid(TargetingPreset)) return;

    UWorld* World = GetWorld();
    UTargetingSubsystem* Subsystem = World ? UTargetingSubsystem::Get(World) : nullptr;
    if (!Subsystem) return;

    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = GetOwner();
    UTargetLockContext* TargetLockContext = NewObject<UTargetLockContext>(this);
    TargetLockContext->CurrentTarget = Cast<AActor>(NearestTarget.GetObject());
    TargetLockContext->Mode = AxisValue.X > 0.f ? ETargetSwitchMode::SwitchRight : ETargetSwitchMode::SwitchLeft;
    SourceContext.SourceObject = TargetLockContext;

    const FTargetingRequestHandle TargetingHandle =
        UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
    const FTargetingRequestDelegate Delegate = FTargetingRequestDelegate::CreateUObject(
        this, &UTargetLockComponent::OnSwitchTargetingCompleted);

    Subsystem->ExecuteTargetingRequestWithHandle(TargetingHandle, Delegate);
}

void UTargetLockComponent::AutoSwitchTarget()
{
    TryStartTargetLock();
}

AActor* UTargetLockComponent::GetLockedOnTargetActor() const
{
    if (NearestTarget == nullptr) return nullptr;
	return GetTargetOwnerActor(NearestTarget);
}

bool UTargetLockComponent::IsLocked() const
{
	return bTargetLocked && NearestTarget;
}

void UTargetLockComponent::ResetIsSwitchingTarget()
{
    if (!SwitchingTargetTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
             SwitchingTargetTimerHandle,
             this,
             &UTargetLockComponent::ResetIsSwitchingTarget,
             bIsSwitchingTarget ? 0.25f : 0.5f,
             false
         );
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(SwitchingTargetTimerHandle);
	bIsSwitchingTarget = false;
}

bool UTargetLockComponent::CanSwitchTarget(const FVector2D& AxisValue) const
{
	return FMath::Abs(AxisValue.X) >= StartRotatingThreshold || FMath::Abs(AxisValue.Y) >= StartRotatingThreshold;
}

void UTargetLockComponent::CreateAndAttachTargetLockedOnWidgetComponent(const TargetInterface Interface)
{
    AActor* TargetActor = GetTargetOwnerActor(Interface);
    if (!IsValid(TargetActor)) return;

    const TArray<UTargetPointComponent*> TargetPoints = Interface->GetTargetPoints();
    if (TargetPoints.IsEmpty()) return;

    // The reticle attaches to the locked point (the target's first point), seeded in
    // StartObservingTarget; fall back to the first available point.
    USceneComponent* AttachPoint = LockedPoint ? LockedPoint : TargetPoints[0];

	if (!LockedOnWidgetClass)
	{
		TS_LOG(Error, TEXT("TargetSystemComponent: Cannot get LockedOnWidgetClass, please ensure it is a valid reference in the Component Properties."));
		return;
	}

	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName(TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn")));
	TargetLockedOnWidgetComponent->SetWidgetClass(LockedOnWidgetClass);

	UMeshComponent* MeshComponent = TargetActor->FindComponentByClass<UMeshComponent>();
	USceneComponent* ParentComponent = MeshComponent ? MeshComponent : TargetActor->GetRootComponent();

	if (IsValid(OwnerPlayerController))
	{
		TargetLockedOnWidgetComponent->SetOwnerPlayer(OwnerPlayerController->GetLocalPlayer());
	}

	TargetLockedOnWidgetComponent->ComponentTags.Add(FName("TargetSystem.LockOnWidget"));
	TargetLockedOnWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TargetLockedOnWidgetComponent->SetupAttachment(AttachPoint);
	TargetLockedOnWidgetComponent->SetRelativeLocation(LockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(LockedOnWidgetDrawSize, LockedOnWidgetDrawSize));
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

void UTargetLockComponent::SetupLocalPlayerController()
{
	if (!IsValid(OwnerPawn))
	{
		TS_LOG(Error, TEXT("[%s] TargetSystemComponent: Component is meant to be added to Pawn only ..."), *GetName());
		return;
	}

	OwnerPlayerController = Cast<APlayerController>(OwnerPawn->GetController());
}

bool UTargetLockComponent::LineTrace(const FVector& Start, const FVector& End, FHitResult& Hit) const
{
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(GetOwner());

	  TArray<AActor*> IgnoredActors {};
    IgnoredActors.Init(OwnerActor, 1);
    for (AActor* ChildActor : OwnerActor->Children)
    {
        IgnoredActors.Add(ChildActor);
    }
    Params.AddIgnoredActors(IgnoredActors);
    GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        TargetCollisionChannel,
        Params
    );

    return Hit.HitObjectHandle.GetLocation() == End;
}

FRotator UTargetLockComponent::GetControlRotationOnTarget(TargetInterface Interface) const
{
    if (!Interface) return FRotator::ZeroRotator;

	if (!IsValid(OwnerPlayerController))
	{
		TS_LOG(Warning, TEXT("UTargetLockComponent::GetControlRotationOnTarget - OwnerPlayerController is not valid ..."))
		return FRotator::ZeroRotator;
	}

	const FRotator ControlRotation = OwnerPlayerController->GetControlRotation();

	const FVector CharacterLocation = OwnerActor->GetActorLocation();
    FVector TargetPointLocation = GetTargetOwnerLocation(Interface);

	// Find look at rotation
	const FRotator LookRotation = FRotationMatrix::MakeFromX(TargetPointLocation - CharacterLocation).Rotator();
	float Pitch = LookRotation.Pitch;
	FRotator TargetRotation;
	if (bAdjustPitchBasedOnDistanceToTargetUsingCurve)
	{
		const float Distance = GetDistanceFromTarget(Interface);

	    // The locked point (the target's first point) drives the per-point pitch curve;
	    // fall back to the component-level DefaultPitchOffsetCurve.
	    const UCurveFloat* CurvePitch =
	        (LockedPoint && IsValid(LockedPoint->GetLockOnPitchOffsetCurve()))
	        ? LockedPoint->GetLockOnPitchOffsetCurve()
	        : DefaultPitchOffsetCurve;

		const float CurveValue = IsValid(CurvePitch) ? CurvePitch->GetFloatValue(Distance) : 0.f;
		TargetRotation = FRotator(CurveValue, LookRotation.Yaw, ControlRotation.Roll);
	}
	else if (bAdjustPitchBasedOnDistanceToTarget)
	{
		const float DistanceToTarget = GetDistanceFromTarget(Interface);
		const float PitchInRange = (DistanceToTarget * PitchDistanceCoefficient + PitchDistanceOffset) * -1.0f;
		const float PitchOffset = FMath::Clamp(PitchInRange, PitchMin, PitchMax);

		Pitch = Pitch + PitchOffset;
		TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}
	else
	{
	    TargetRotation = FRotator(Pitch, LookRotation.Yaw, ControlRotation.Roll);
	}

	return FMath::RInterpTo(ControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 9.0f);
}

AActor* UTargetLockComponent::GetTargetOwnerActor(const TargetInterface& Interface) const
{
    if (!Interface) return nullptr;
    return Cast<AActor>(Interface.GetObject());
}

FVector UTargetLockComponent::GetTargetOwnerLocation(const TargetInterface& Interface) const
{
    const AActor* TargetActor = GetTargetOwnerActor(Interface);
    if (!IsValid(TargetActor)) return FVector::Zero();
    return TargetActor->GetActorLocation();
}

void UTargetLockComponent::SetControlRotationOnTarget() const
{
	if (!IsValid(OwnerPlayerController)) return;
    if (!NearestTarget) return;

	const FRotator ControlRotation = GetControlRotationOnTarget(NearestTarget);
    OwnerPlayerController->SetControlRotation(ControlRotation);
}

float UTargetLockComponent::GetDistanceFromTarget(const TargetInterface& Interface) const
{
	return OwnerActor->GetDistanceTo(GetTargetOwnerActor(Interface));
}

void UTargetLockComponent::ControlRotation(const bool ShouldControlRotation) const
{
	if (!IsValid(OwnerPawn))
	{
		return;
	}

    if (CharacterRotationMode == ECharacterRotationMode::OrientToMovement)
    {
        OwnerPawn->bUseControllerRotationYaw = ShouldControlRotation;

        UCharacterMovementComponent* CharacterMovementComponent = OwnerPawn->FindComponentByClass<UCharacterMovementComponent>();
        if (IsValid(CharacterMovementComponent))
        {
            CharacterMovementComponent->bOrientRotationToMovement = !ShouldControlRotation;
        }
    }
}
