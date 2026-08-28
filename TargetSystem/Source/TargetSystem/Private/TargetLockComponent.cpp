// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetLockComponent.h"
#include "TargetPointComponent.h"
#include "TargetSystemInterface.h"
#include "Components/WidgetComponent.h"
#include "EngineUtils.h"
#include "TargetSystemLog.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
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
    NearestTarget->OnTargetLockBegin(GetOwner());

    // Lock onto the point the player is actually aiming at (smallest camera angle, distance as a
    // tiebreak) rather than the array's first entry — drives the reticle widget attach and the
    // per-point pitch curve. Array order is authoring order (head/wing/tail as added), so [0] would
    // lock a near-random point; SelectBestLockOnPoint mirrors SortByLockOn's angle+distance scoring.
    const TArray<UTargetPointComponent*> TargetPoints = NearestTarget->GetTargetPoints();
    LockedPoint = SelectBestLockOnPoint(TargetPoints);

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
    // Lock may have been torn down between timer ticks.
    if (!NearestTarget)
    {
        StopObservingTarget(false, true);
        return;
    }

    // Hold the lock while the target is alive and in range — no auto-switch, no re-pick, no
    // line-of-sight check (Req 2: target selection lives entirely in the TargetingPreset).
    if (NearestTarget->IsTargetable()
        && GetDistanceFromTarget(NearestTarget) <= LoseTargetDistance) return;

    // Death / out-of-range: drop and re-select through the preset (auto-switch on death).
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

        NearestTarget->OnTargetLockEnd(GetOwner());
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

#if TARGETSYSTEM_WITH_DEBUG
void UTargetLockComponent::CaptureDebugCandidates(FTargetingRequestHandle Handle, TArray<FTargetLockDebugCandidate>& Out) const
{
    Out.Reset();

    const FTargetingDefaultResultsSet* Results = FTargetingDefaultResultsSet::Find(Handle);
    if (!Results)
    {
        return;
    }

    bool bWinnerMarked = false;
    for (const FTargetingDefaultResultData& Data : Results->TargetResults)
    {
        AActor* Actor = Data.HitResult.GetActor();
        if (!IsValid(Actor))
        {
            continue;
        }

        FTargetLockDebugCandidate& Candidate = Out.AddDefaulted_GetRef();
        Candidate.Actor = Actor;
        Candidate.Score = Data.Score;

        if (!bWinnerMarked)
        {
            Candidate.bWinner = true;
            bWinnerMarked = true;
        }
    }
}
#endif

void UTargetLockComponent::OnTargetingCompleted(FTargetingRequestHandle Handle)
{
    PotentialTargets.Reset();
    AActor* First = ExtractTargetingResults(Handle, PotentialTargets);

#if TARGETSYSTEM_WITH_DEBUG
    CaptureDebugCandidates(Handle, DebugLockOnCandidates);
    DebugLastMode = ETargetSwitchMode::LockOn;
#endif

    if (!First)
    {
        // No target found — initial lock-on came up empty, or the death auto-switch (Req 2) found
        // no other combatant. Either way fully tear the lock down (clears a dead NearestTarget,
        // restores look input). StopTargetLock broadcasts OnFinishTargetLock at the end.
        StopTargetLock();
        return;
    }

    NearestTarget = First;
    StartObservingTarget();
}

void UTargetLockComponent::OnSwitchTargetingCompleted(FTargetingRequestHandle Handle)
{
    TArray<TargetInterface> Results;
    AActor* First = ExtractTargetingResults(Handle, Results);

#if TARGETSYSTEM_WITH_DEBUG
    CaptureDebugCandidates(Handle, DebugSwitchCandidates);
    if (const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(Handle))
    {
        if (const UTargetLockContext* LockContext = Cast<UTargetLockContext>(SourceContext->SourceObject))
        {
            DebugLastMode = LockContext->Mode;
        }
    }
#endif

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
        TS_LOG(Warning, TEXT("[%s] TargetLockComponent: TargetingPreset is not assigned — lock-on cannot run. Assign a TargetingPreset on the component."), *GetName());
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

bool UTargetLockComponent::ExecuteTargetSwitch(int32 Direction)
{
    // Core of the enemy (left/right) switch, shared by SwitchTarget and the SwitchTargetPoint
    // cross-target overflow. Runs the TargetingPreset synchronously in SwitchLeft/Right mode
    // (FilterSwitchTargetSide + SortByLockOn branch on the Mode) and reports whether the locked
    // target actually changed (false => no neighbour on that side, e.g. the global edge).
    if (!IsValid(TargetingPreset))
    {
        TS_LOG(Warning, TEXT("[%s] TargetLockComponent: TargetingPreset is not assigned — target switch cannot run."), *GetName());
        return false;
    }

    UWorld* World = GetWorld();
    UTargetingSubsystem* Subsystem = World ? UTargetingSubsystem::Get(World) : nullptr;
    if (!Subsystem) return false;

    const AActor* Before = GetLockedOnTargetActor();

    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = GetOwner();
    UTargetLockContext* TargetLockContext = NewObject<UTargetLockContext>(this);
    TargetLockContext->CurrentTarget = Cast<AActor>(NearestTarget.GetObject());
    TargetLockContext->Mode = Direction > 0 ? ETargetSwitchMode::SwitchRight : ETargetSwitchMode::SwitchLeft;
    SourceContext.SourceObject = TargetLockContext;

    const FTargetingRequestHandle TargetingHandle =
        UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
    const FTargetingRequestDelegate Delegate = FTargetingRequestDelegate::CreateUObject(
        this, &UTargetLockComponent::OnSwitchTargetingCompleted);

    // Synchronous: OnSwitchTargetingCompleted updates NearestTarget before this returns.
    Subsystem->ExecuteTargetingRequestWithHandle(TargetingHandle, Delegate);

    return GetLockedOnTargetActor() != Before;
}

void UTargetLockComponent::SwitchTarget(FVector2D AxisValue)
{
    if (!bTargetLocked) return;

	// Magnitude gating happens at the input layer (MinimumMagnitudeToSwitchTarget). Here we only
	// debounce repeats: bIsSwitchingTarget holds for SwitchCooldown seconds after each switch so a
	// held stick / continued mouse motion can't step through several targets in one gesture.
	if (bIsSwitchingTarget) return;

    // Req 3: manual enemy switch (Mode = SwitchLeft/Right). The core lives in ExecuteTargetSwitch so
    // the unified point-scroll (SwitchTargetPoint) can reuse it for cross-target overflow.
    ExecuteTargetSwitch(AxisValue.X > 0.f ? 1 : -1);
}

UTargetPointComponent* UTargetLockComponent::RunSwitchPointRequest(AActor* TargetActor, UTargetPointComponent* StartPoint, int32 Direction)
{
    // Drive point selection through the subsystem. SelectTargetPoint (in SwitchPointPreset) reads
    // the context in SwitchPoint mode and writes the chosen point back to Ctx->CurrentPoint — the
    // round-trip channel for the result. StartPoint == null => land on the entry-edge point;
    // a returned point equal to StartPoint => we are at the target edge (boundary contract).
    UWorld* World = GetWorld();
    UTargetingSubsystem* Subsystem = World ? UTargetingSubsystem::Get(World) : nullptr;
    if (!Subsystem) return StartPoint;

    FTargetingSourceContext SourceContext;
    SourceContext.SourceActor = GetOwner();
    UTargetLockContext* Ctx = NewObject<UTargetLockContext>(this);
    Ctx->Mode = ETargetSwitchMode::SwitchPoint;
    Ctx->CurrentTarget = TargetActor;
    Ctx->CurrentPoint = StartPoint;
    Ctx->SwitchDirection = Direction;
    SourceContext.SourceObject = Ctx;

    const FTargetingRequestHandle TargetingHandle =
        UTargetingSubsystem::MakeTargetRequestHandle(SwitchPointPreset, SourceContext);
    const FTargetingRequestDelegate Delegate = FTargetingRequestDelegate::CreateUObject(
        this, &UTargetLockComponent::OnSwitchPointTargetingCompleted);

    // Synchronous: Ctx->CurrentPoint is populated by the task by the time this returns.
    Subsystem->ExecuteTargetingRequestWithHandle(TargetingHandle, Delegate);

    return Ctx->CurrentPoint;
}

void UTargetLockComponent::SwitchTargetPoint(FVector2D AxisValue)
{
    if (!bTargetLocked || !NearestTarget) return;

	// Repeat-protection: the project calls this every frame from analog look input
	// (UGInputHandler_Look::PostProcessLookInput), so without a cooldown one gesture would step
	// through several points/targets in a row.
    if (bIsSwitchingTarget) return;

    if (!IsValid(SwitchPointPreset))
    {
        TS_LOG(Warning, TEXT("[%s] TargetLockComponent: SwitchPointPreset is not assigned — point switch cannot run."), *GetName());
        return;
    }

    const int32 Direction = AxisValue.X > 0.f ? 1 : -1;

    // 1) Step along the CURRENT target's points. SelectTargetPoint returns the locked point
    //    unchanged when we are already at the target edge (the boundary contract).
    UTargetPointComponent* Stepped = RunSwitchPointRequest(GetTargetOwnerActor(NearestTarget), LockedPoint, Direction);
    if (IsValid(Stepped) && Stepped != LockedPoint)
    {
        // LockedPoint drives the reticle attach and the camera focus + pitch curve
        // (GetLockedFocusLocation / GetControlRotationOnTarget read it on the next tick).
        LockedPoint = Stepped;
        MoveReticleToLockedPoint();
    	bIsSwitchingTarget = true;
    	ResetIsSwitchingTarget();
        return;
    }

    // 2) Target edge reached (or 0–1 point target): overflow onto the adjacent enemy on that side.
    //    On the global edge there is no neighbour => stay on the current point (clamp, no wrap).
    if (!ExecuteTargetSwitch(Direction))
    {
        return;
    }

    // 3) Landed on the new target (NearestTarget / LockedPoint updated by the switch). Re-seat the
    //    lock onto its entry-edge point so a continued scroll keeps flowing the same way — a null
    //    start point tells SelectTargetPoint to pick the entry edge instead of stepping.
    UTargetPointComponent* Entry = RunSwitchPointRequest(GetTargetOwnerActor(NearestTarget), nullptr, Direction);
    if (IsValid(Entry))
    {
        LockedPoint = Entry;
        MoveReticleToLockedPoint();
    }
}

void UTargetLockComponent::OnSwitchPointTargetingCompleted(FTargetingRequestHandle Handle)
{
    // No-op: SwitchTargetPoint reads the chosen point inline from the context after the
    // synchronous request returns. Kept for delegate symmetry with the other completion handlers.
}

void UTargetLockComponent::AutoSwitchTarget()
{
    // Req 2: on the current target's death, re-select through the TargetingPreset (it picks the new
    // nearest valid target). If the preset yields nobody, OnTargetingCompleted tears the lock down
    // via StopTargetLock. All target-selection logic stays in the preset — the plugin stays dumb.
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

float UTargetLockComponent::GetCameraParallaxYawOffset() const
{
    if (!IsLocked() || !IsValid(OwnerActor) || !IsValid(OwnerPlayerController)
        || !IsValid(OwnerPlayerController->PlayerCameraManager))
    {
        return 0.f;
    }

    // Difference between "where the camera has to look to centre the target" and "where the body is
    // aimed" (GetControlRotationOnTarget's pivot yaw). No feedback loop: ControlRotation no longer
    // depends on the camera location, so this only ever reads the rig, never steers it.
    const FVector FocusLocation  = GetLockedFocusLocation(NearestTarget);
    const FVector PivotLocation  = OwnerActor->GetActorLocation();
    const FVector CameraLocation = OwnerPlayerController->PlayerCameraManager->GetCameraLocation();

    const float PivotYaw  = FRotationMatrix::MakeFromX(FocusLocation - PivotLocation).Rotator().Yaw;
    const float CameraYaw = FRotationMatrix::MakeFromX(FocusLocation - CameraLocation).Rotator().Yaw;

    return FMath::FindDeltaAngleDegrees(PivotYaw, CameraYaw);
}

void UTargetLockComponent::ResetIsSwitchingTarget()
{
    if (!SwitchingTargetTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().SetTimer(
             SwitchingTargetTimerHandle,
             this,
             &UTargetLockComponent::ResetIsSwitchingTarget,
             SwitchCooldown,
             false
         );
        return;
    }

    GetWorld()->GetTimerManager().ClearTimer(SwitchingTargetTimerHandle);
	bIsSwitchingTarget = false;
}

void UTargetLockComponent::CreateAndAttachTargetLockedOnWidgetComponent(const TargetInterface Interface)
{
    AActor* TargetActor = GetTargetOwnerActor(Interface);
    if (!IsValid(TargetActor)) return;

    const TArray<UTargetPointComponent*> TargetPoints = Interface->GetTargetPoints();

    // R8: target points are optional — lock-on works without them. Prefer the locked point,
    // then the first point; if the target has none, attach the reticle to its mesh (or root)
    // so the widget still shows at the actor location instead of silently disappearing.
    USceneComponent* AttachPoint = nullptr;
    if (LockedPoint)
    {
        AttachPoint = LockedPoint.Get();
    }
    else if (!TargetPoints.IsEmpty())
    {
        AttachPoint = TargetPoints[0];
    }
    else if (UMeshComponent* Mesh = TargetActor->FindComponentByClass<UMeshComponent>())
    {
        AttachPoint = Mesh;
    }
    else
    {
        AttachPoint = TargetActor->GetRootComponent();
    }
    if (!IsValid(AttachPoint)) return;

	if (!LockedOnWidgetClass)
	{
		TS_LOG(Error, TEXT("TargetSystemComponent: Cannot get LockedOnWidgetClass, please ensure it is a valid reference in the Component Properties."));
		return;
	}

	TargetLockedOnWidgetComponent = NewObject<UWidgetComponent>(TargetActor, MakeUniqueObjectName(TargetActor, UWidgetComponent::StaticClass(), FName("TargetLockOn")));
	TargetLockedOnWidgetComponent->SetWidgetClass(LockedOnWidgetClass);

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

void UTargetLockComponent::MoveReticleToLockedPoint()
{
    if (!IsValid(TargetLockedOnWidgetComponent) || !LockedPoint) return;

    TargetLockedOnWidgetComponent->AttachToComponent(
        LockedPoint, FAttachmentTransformRules::KeepRelativeTransform);
    TargetLockedOnWidgetComponent->SetRelativeLocation(LockedOnWidgetRelativeLocation);
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
    FVector TargetPointLocation = GetLockedFocusLocation(Interface);

	// Yaw MUST be aimed from the character pivot, not from the camera position. ControlRotation is the
	// combat aim: bUseControllerDesiredRotation turns the body to it, and melee traces / motion warping
	// inherit that facing. Aiming it from an over-the-shoulder camera creates a feedback loop
	// (ControlRotation -> spring arm -> camera location -> ControlRotation) whose fixed point centres the
	// target on screen while leaving the BODY off by atan2(SideOffset, Distance) — ~23 deg at 1.5 m, which
	// is a whiffed hit. Screen centring is a camera concern: read GetCameraParallaxYawOffset() from a
	// camera modifier / rig and apply it to the view only, never to ControlRotation.
	// Find look at rotation.
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

	    // Req 4 (high-ground): drive the upward tilt from the GROUND height difference between the two
	    // actor origins, NOT from the look-at to the locked point. The look-at to a chest/head point is
	    // steeply positive at close range even for a same-level foe (atan of a tiny horizontal distance),
	    // which craned the camera under the target as the player closed in. Comparing the two origins'
	    // Z (same reference height) gives ~0 for a same-level enemy at ANY distance, and a real positive
	    // angle only when the enemy stands higher. Clamped to >= 0 (only upward) and capped so a foe
	    // almost directly overhead can't flip the camera fully vertical.
	    const FVector TargetActorLocation = GetTargetOwnerLocation(Interface);
	    const float HeightDelta = TargetActorLocation.Z - CharacterLocation.Z;
	    const float HorizontalDistance = FVector::Dist2D(TargetActorLocation, CharacterLocation);
	    const float ElevationPitch = FMath::Clamp(
	        FMath::RadiansToDegrees(FMath::Atan2(HeightDelta, FMath::Max(HorizontalDistance, 1.f))),
	        0.f, HighGroundMaxPitch);
		TargetRotation = FRotator(CurveValue + ElevationPitch, LookRotation.Yaw, ControlRotation.Roll);
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

FVector UTargetLockComponent::GetLockedFocusLocation(const TargetInterface& Interface) const
{
    // Aim at the locked point on the current target (UC3); otherwise the actor origin.
    if (Interface.GetObject() == NearestTarget.GetObject() && IsValid(LockedPoint))
    {
        return LockedPoint->GetComponentLocation();
    }
    return GetTargetOwnerLocation(Interface);
}

UTargetPointComponent* UTargetLockComponent::SelectBestLockOnPoint(
    const TArray<UTargetPointComponent*>& Points) const
{
    if (Points.IsEmpty())
    {
        return nullptr;
    }

    // Mirror SortByLockOn::ComputeLockOnScore, but ranking the chosen target's POINTS instead of
    // actors: the point with the smallest camera angle wins (distance only breaks ties), so the lock
    // lands on the point the player is looking at. Prefer the camera (the lock-on view); fall back to
    // the owner pawn's forward when the camera manager is unavailable.
    FVector ViewLocation;
    FVector ViewForward;
    if (IsValid(OwnerPlayerController) && IsValid(OwnerPlayerController->PlayerCameraManager))
    {
        ViewLocation = OwnerPlayerController->PlayerCameraManager->GetCameraLocation();
        ViewForward  = OwnerPlayerController->PlayerCameraManager->GetCameraRotation().Vector();
    }
    else if (IsValid(OwnerActor))
    {
        ViewLocation = OwnerActor->GetActorLocation();
        ViewForward  = OwnerActor->GetActorForwardVector();
    }
    else
    {
        return Points[0];
    }
    ViewForward = ViewForward.GetSafeNormal();

    const FVector SourceLocation = IsValid(OwnerActor) ? OwnerActor->GetActorLocation() : ViewLocation;

    UTargetPointComponent* BestPoint = nullptr;
    float BestScore = TNumericLimits<float>::Max();
    for (UTargetPointComponent* Point : Points)
    {
        if (!IsValid(Point))
        {
            continue;
        }

        const FVector PointLocation = Point->GetComponentLocation();
        const FVector ToPoint = (PointLocation - ViewLocation).GetSafeNormal();
        const float Dot = FMath::Clamp(FVector::DotProduct(ViewForward, ToPoint), -1.f, 1.f);
        const float AngleDegrees = FMath::RadiansToDegrees(FMath::Acos(Dot));
        const float Distance = FVector::Distance(SourceLocation, PointLocation);

        // Angle dominates (matches the asset's AngleWeight > DistanceWeight); the small distance term
        // (~1 angle-degree per 100 uu) only decides between points at a near-equal camera angle.
        const float Score = AngleDegrees + Distance * 0.01f;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestPoint = Point;
        }
    }

    return IsValid(BestPoint) ? BestPoint : Points[0];
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
