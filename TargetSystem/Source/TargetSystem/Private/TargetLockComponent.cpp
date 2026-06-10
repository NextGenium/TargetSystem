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
    RequiredClass = APawn::StaticClass();
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

    if (LoseTargetDistance < MaximumDistanceCanStartTarget)
    {
        LoseTargetDistance = MaximumDistanceCanStartTarget;
    }

	SetupLocalPlayerController();
}

void UTargetLockComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bTargetLocked || !NearestTarget) return;

    SetControlRotationOnTarget();
}

bool UTargetLockComponent::CanTargetLock() const
{
    return !PotentialTargets.IsEmpty();
}

void UTargetLockComponent::StartObservingTarget()
{
    bTargetLocked = true;
    NearestTarget->StartTargetable();

    // Interim: the tag-driven StartTargetPointName is gone with FTargetActorDetails;
    // default the focus socket to the first lock-on point. Replaced in Step 8.
    const TArray<UTargetPointComponent*> TargetPoints = NearestTarget->GetTargetPoints();
    CurrentSocketOnNearestTarget = TargetPoints.IsEmpty() ? FString() : TargetPoints[0]->GetName();

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
    LockedPoint = nullptr;

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

    // Best-effort: if a SelectTargetPoint task baked a point component into the first
    // result, track it for the pitch-offset curve. Null with the single lock-on preset.
    if (const FTargetingDefaultResultsSet* Results = FTargetingDefaultResultsSet::Find(Handle))
    {
        if (Results->TargetResults.Num() > 0)
        {
            LockedPoint = Cast<UTargetPointComponent>(Results->TargetResults[0].HitResult.Component.Get());
        }
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
    if (bUseTargetSubsystem)
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
        return;
    }

    // Manual fallback (Souls-like search). Retires once the subsystem path is verified.
    AddPotentialTargetsByInterface(RequiredClass);
    if (!CanTargetLock())
    {
       MessageFinishTargetLock();
        return;
    }

    NearestTarget = FindNearestTarget(true);
    if (!NearestTarget)
    {
        MessageFinishTargetLock();
        return;
    }

    StartObservingTarget();
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

    if (bUseTargetSubsystem)
    {
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
        return;
    }

    // Manual fallback.
    if (TrySwitchBetweenTargetPoints(AxisValue)) return;
    if (PotentialTargets.Num() <= 1) return;
    if (bIsSwitchingTarget) return;

    TArray<TargetInterface> ActorsToLook;
    FHitResult Hit;

    for (TargetInterface Interface : PotentialTargets)
    {
        if(!LineTrace(GetOwner()->GetActorLocation(),  GetTargetOwnerLocation(Interface), Hit)) continue;
        if (!IsInViewport(Interface)) continue;

        ActorsToLook.Add(Interface);
    }

    const TargetInterface NewTarget = FMath::Abs(AxisValue.X) > FMath::Abs(AxisValue.Y)  ?
        FindByHorizontal(ActorsToLook, AxisValue.X) :
        FindByVertical(ActorsToLook, AxisValue);

    if (!NewTarget) return;

    bIsSwitchingTarget = true;

    StopObservingTarget();
    NearestTarget = NewTarget;
    StartObservingTarget();

    ResetIsSwitchingTarget();
}

void UTargetLockComponent::AutoSwitchTarget()
{
    if (bUseTargetSubsystem)
    {
        TryStartTargetLock();
        return;
    }

    const TScriptInterface<ITargetSystemInterface> NewTarget = FindNearestTarget();
    if (!NewTarget)
    {
        StopTargetLock();
        return;
    }
    NearestTarget = NewTarget;
    StartObservingTarget();

    bIsSwitchingTarget = true;
    ResetIsSwitchingTarget();
}

bool UTargetLockComponent::TrySwitchBetweenTargetPoints(FVector2D AxisValue)
{
    if (!NearestTarget) return false;

    const TArray<UTargetPointComponent*> TargetPoints = NearestTarget->GetTargetPoints();
    if (TargetPoints.Num() <= 1) return false;
    if (bIsSwitchingTarget) return false;

    const int32 MaxIndex = TargetPoints.Num() - 1;
    const float MajorAxis = FMath::Abs(AxisValue.X) > FMath::Abs(AxisValue.Y) ? AxisValue.X : AxisValue.Y;

    AActor* TargetOwner = GetTargetOwnerActor(NearestTarget);
    if (!IsValid(TargetOwner)) return false;

    const float RangeMin = TargetOwner->GetActorRotation().Yaw - 90.f;
    const float RangeMax = TargetOwner->GetActorRotation().Yaw + 90.f;

    const int32 SwitchDirection = OwnerActor->GetActorRotation().Yaw > RangeMin && OwnerActor->GetActorRotation().Yaw < RangeMax ?
          MajorAxis > 0.f ? 1 : -1:
          MajorAxis > 0.f ? -1 : 1;


    int32 CurrentIndex = 0;
    for (int32 i = 0; i < TargetPoints.Num(); ++i)
    {
        if (TargetPoints[i]->GetName() == CurrentSocketOnNearestTarget)
        {
            CurrentIndex = i; break;
        }
    }

    const int32 NewIndex = CurrentIndex + SwitchDirection;
    if (NewIndex > MaxIndex || NewIndex < 0) return false;

    CurrentSocketOnNearestTarget = TargetPoints[NewIndex]->GetName();
    if (TargetLockedOnWidgetComponent)
    {
        TargetLockedOnWidgetComponent->DestroyComponent();
    }
    CreateAndAttachTargetLockedOnWidgetComponent(NearestTarget);
    bIsSwitchingTarget = true;
    ResetIsSwitchingTarget();
    return true;
}

TScriptInterface<ITargetSystemInterface> UTargetLockComponent::FindByHorizontal(TArray<TargetInterface> LookTargets, float AxisValue) const
{
    TScriptInterface<ITargetSystemInterface> NewNearestTarget = nullptr;

    float MinDistance = MaximumDistanceCanStartTarget;
    const float RangeMin = AxisValue < 0 ? 0 : 180;
    const float RangeMax = AxisValue < 0 ? 180 : 360;

    for (TScriptInterface<ITargetSystemInterface> Interface : LookTargets)
    {
        if (NearestTarget == Interface) continue;
        const float Angle = GetAngleUsingCameraRotation(GetTargetOwnerLocation(Interface));
        if (Angle < RangeMin || Angle > RangeMax) continue;

        const float Distance = GetDistanceFromTarget(Interface);
        if (Distance > MaximumDistanceCanStartTarget) continue;

        const float RelativeActorsDistance = GetTargetOwnerActor(NearestTarget)->GetDistanceTo(GetTargetOwnerActor(Interface));
        if (RelativeActorsDistance > MinDistance) continue;

        MinDistance = RelativeActorsDistance;
        NewNearestTarget = Interface;
    }
    return NewNearestTarget;
}

TScriptInterface<ITargetSystemInterface> UTargetLockComponent::FindByVertical(TArray<TargetInterface> LookTargets, FVector2D AxisValue) const
{
    TScriptInterface<ITargetSystemInterface> NewNearestTarget = nullptr;

    float MinDistance = MaximumDistanceCanStartTarget;

    const float RangeMin = AxisValue.X < 0.f ? 0 : 180;
    const float RangeMax = AxisValue.X < 0.f ? 180 : 360;

    for (TScriptInterface<ITargetSystemInterface> Interface : LookTargets)
    {
        if (NearestTarget == Interface) continue;

        const float Angle = GetAngleUsingCameraRotation(GetTargetOwnerLocation(Interface));
        if (Angle < RangeMin || Angle > RangeMax) continue;

        const float Distance = GetDistanceFromTarget(Interface);
        if (Distance > MaximumDistanceCanStartTarget) continue;

        const float RelativeActorsDistance = GetTargetOwnerActor(NearestTarget)->GetDistanceTo(GetTargetOwnerActor(Interface));
        if (RelativeActorsDistance > MinDistance) continue;

        if (AxisValue.Y < 0.f)
        {
            if (GetDistanceFromTarget(Interface) < GetDistanceFromTarget(NearestTarget)) continue;
        }
        else
        {
            if (GetDistanceFromTarget(Interface) > GetDistanceFromTarget(NearestTarget)) continue;
        }

        MinDistance = RelativeActorsDistance;
        NewNearestTarget = Interface;
    }
    return NewNearestTarget;
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

float UTargetLockComponent::GetAngleUsingCameraRotation(const FVector& Location) const
{
    const UCameraComponent* CameraComponent = OwnerActor->FindComponentByClass<UCameraComponent>();
    if (!IsValid(CameraComponent))
    {
        return GetAngleUsingCharacterRotation(Location);
    }

    const FRotator CameraWorldRotation = CameraComponent->GetComponentRotation();
    const FRotator LookAtRotation = FindLookAtRotation(CameraComponent->GetComponentLocation(), Location);

    float YawAngle = CameraWorldRotation.Yaw - LookAtRotation.Yaw;
    if (YawAngle < 0)
    {
        YawAngle = YawAngle + 360;
    }

    return YawAngle;
}

float UTargetLockComponent::GetAngleUsingCharacterRotation(const FVector& Location) const
{
    const FRotator CharacterRotation = OwnerActor->GetActorRotation();
    const FRotator LookAtRotation = FindLookAtRotation(OwnerActor->GetActorLocation(), Location);

    float YawAngle = CharacterRotation.Yaw - LookAtRotation.Yaw;
    if (YawAngle < 0)
    {
        YawAngle = YawAngle + 360;
    }

    return YawAngle;
}

FRotator UTargetLockComponent::FindLookAtRotation(const FVector Start, const FVector Target)
{
	return FRotationMatrix::MakeFromX(Target - Start).Rotator();
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

    int32 Index = 0;
    for (int32 i = 0; i < TargetPoints.Num(); ++i)
    {
        if (TargetPoints[i]->GetName() == CurrentSocketOnNearestTarget)
        {
            Index = i;
            break;
        }
    }

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
	TargetLockedOnWidgetComponent->SetupAttachment(TargetPoints[Index]);
	TargetLockedOnWidgetComponent->SetRelativeLocation(LockedOnWidgetRelativeLocation);
	TargetLockedOnWidgetComponent->SetDrawSize(FVector2D(LockedOnWidgetDrawSize, LockedOnWidgetDrawSize));
	TargetLockedOnWidgetComponent->SetVisibility(true);
	TargetLockedOnWidgetComponent->RegisterComponent();
}

void UTargetLockComponent::AddPotentialTargetsByInterface(const TSubclassOf<AActor>& ActorClass)
{
	for (TActorIterator ActorIterator(GetWorld(), ActorClass); ActorIterator; ++ActorIterator)
	{
	    TScriptInterface<ITargetSystemInterface> Interface = TScriptInterface<ITargetSystemInterface>(*ActorIterator);
	    if(!ObjectIsTargetable(Interface)) continue;

	    if (GetDistanceFromTarget(Interface) > MaximumDistanceToPotentialTargets) continue;

        PotentialTargets.Add(Interface);
	}
}

bool UTargetLockComponent::ObjectIsTargetable(const TScriptInterface<ITargetSystemInterface> Actor) const
{
    if(!Actor) return false;
    return Actor->IsTargetable();
}

int32 UTargetLockComponent::GetPointIndexByName(const FString& Name) const
{
    constexpr int32 InvalidIndex = -1;
    if (!NearestTarget) return InvalidIndex;

    const TArray<UTargetPointComponent*> TargetPoints = NearestTarget->GetTargetPoints();
    for (int32 i = 0; i < TargetPoints.Num(); ++i)
    {
        if (TargetPoints[i]->GetName() != Name) continue;

        return i;
    }
    return InvalidIndex;
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

void UTargetLockComponent::SortPotentialTargetsByDistance(TArray<TScriptInterface<ITargetSystemInterface>>& Array)
{
    if (Array.IsEmpty()) return;

    Array.Sort([this](const TScriptInterface<ITargetSystemInterface> A, const TScriptInterface<ITargetSystemInterface> B)
        {
            return GetDistanceFromTarget(A) < GetDistanceFromTarget(B);
        }
    );
}

void UTargetLockComponent::SortPotentialTargetsByAngle(TArray<TScriptInterface<ITargetSystemInterface>>& Array)
{
    if (Array.IsEmpty()) return;

    Array.Sort([this](const TScriptInterface<ITargetSystemInterface> A, const TScriptInterface<ITargetSystemInterface> B)
        {
            return GetAngleUsingCameraRotation(GetTargetOwnerLocation(A)) < GetAngleUsingCameraRotation(GetTargetOwnerLocation(B));
        }
    );
}

TScriptInterface<ITargetSystemInterface> UTargetLockComponent::FindNearestTarget(bool bUseAngle)
{
    if (PotentialTargets.IsEmpty()) return nullptr;
    SortPotentialTargetsByDistance(PotentialTargets);

    TArray<TScriptInterface<ITargetSystemInterface>> CopyPotentialTargets = {};
    bool bFindNearestTarget = false;
    int32 BestTargetByDistance_Index = -1;

    for (int32 i = 0; i < PotentialTargets.Num(); ++i)
    {
        FHitResult Hit;
        if(!LineTrace(GetOwner()->GetActorLocation(), GetTargetOwnerLocation(PotentialTargets[i]), Hit)) continue;

        const float Distance = GetDistanceFromTarget(PotentialTargets[i]);

        if (Distance > MaximumDistanceCanStartTarget) continue;

        if (!bIgnoreViewport && !IsInViewport(PotentialTargets[i]) && Distance > DangerousDistanceToTarget) continue;

        if (!bFindNearestTarget)
        {
            bFindNearestTarget = true;
            BestTargetByDistance_Index = i;
        }

        if (!bUseAngle) break;

        CopyPotentialTargets.Add(PotentialTargets[i]);
    }
    if (BestTargetByDistance_Index < 0) return nullptr;
    if (!bUseAngle || bIgnoreViewport) return PotentialTargets[BestTargetByDistance_Index];

    SortPotentialTargetsByAngle(CopyPotentialTargets);

    for (int32 i = 0; i < CopyPotentialTargets.Num(); ++i)
    {
        if (GetAngleUsingCameraRotation(GetTargetOwnerLocation(CopyPotentialTargets[i])) > MaximumFindAngle) continue;

        const float Distance = GetDistanceFromTarget(CopyPotentialTargets[i]);
        if (GetDistanceFromTarget(PotentialTargets[BestTargetByDistance_Index]) + ExtraDistanceToLimitWhenSearchingByAngle < Distance) continue;

        return CopyPotentialTargets[i];
    }
    return PotentialTargets[BestTargetByDistance_Index];
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

	    // Subsystem path bakes the locked point into LockedPoint (via SelectTargetPoint);
	    // the manual fallback resolves it by the FString socket index. Either way fall back
	    // to the component-level DefaultPitchOffsetCurve.
	    const UCurveFloat* CurvePitch = DefaultPitchOffsetCurve;
	    if (LockedPoint && IsValid(LockedPoint->GetLockOnPitchOffsetCurve()))
	    {
	        CurvePitch = LockedPoint->GetLockOnPitchOffsetCurve();
	    }
	    else
	    {
	        const int32 Index = GetPointIndexByName(CurrentSocketOnNearestTarget);
	        const TArray<UTargetPointComponent*> TargetPoints = Interface->GetTargetPoints();
	        if (TargetPoints.IsValidIndex(Index) && IsValid(TargetPoints[Index]->GetLockOnPitchOffsetCurve()))
	        {
	            CurvePitch = TargetPoints[Index]->GetLockOnPitchOffsetCurve();
	        }
	    }

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

bool UTargetLockComponent::IsInViewport(TargetInterface Interface) const
{
	if (!IsValid(OwnerPlayerController)) return true;

	FVector2D ScreenLocation;
	OwnerPlayerController->ProjectWorldLocationToScreen(GetTargetOwnerLocation(Interface), ScreenLocation);

	FVector2D ViewportSize;
	GetWorld()->GetGameViewport()->GetViewportSize(ViewportSize);

	return ScreenLocation.X > 10.f && ScreenLocation.Y > 10.f && ScreenLocation.X < ViewportSize.X && ScreenLocation.Y < ViewportSize.Y;
}
