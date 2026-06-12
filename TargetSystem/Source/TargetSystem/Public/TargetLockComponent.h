// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetSystemInterface.h"
#include "TargetPointQuery.h"
#include "Components/ActorComponent.h"

// Debug instrumentation (candidate scoring + lock internals) is compiled only when a debug
// consumer exists: the Gameplay Debugger category or the targetsystem.DebugDraw cvar overlay.
// Both expand to 0 in shipping, so the cache, getters and capture code drop out entirely.
#define TARGETSYSTEM_WITH_DEBUG (WITH_GAMEPLAY_DEBUGGER || ENABLE_DRAW_DEBUG)

#if TARGETSYSTEM_WITH_DEBUG
#include "Targeting/TargetLockContext.h" // ETargetSwitchMode for the captured candidate sets
#endif

#include "TargetLockComponent.generated.h"

using TargetInterface = TScriptInterface<ITargetSystemInterface>;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishTargetLock);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnTargetIsDead,
    TScriptInterface<ITargetSystemInterface>, Interface
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FComponentOnTargetLockedOnOff,
    const AActor*, TargetActor
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FComponentSetRotation,
    const AActor*, TargetActor,
    FRotator&, ControlRotation
);

UENUM(BlueprintType)
enum class ECharacterRotationMode : uint8
{
    OrientToMovement,
    Strafe,
};

class UUserWidget;
class UWidgetComponent;
class APlayerController;
class UTargetingPreset;
class UTargetPointComponent;
struct FTargetingRequestHandle;

#if TARGETSYSTEM_WITH_DEBUG
// One scored entry from the last targeting request, captured for the debug overlays
// (GameplayDebugger category + targetsystem.DebugDraw). Results arrive already sorted, so the
// first captured entry is the winner the component locked / switched to.
struct FTargetLockDebugCandidate
{
	TWeakObjectPtr<AActor> Actor;
	float Score = 0.f;
	bool  bWinner = false;
};
#endif

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TARGETSYSTEM_API UTargetLockComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTargetLockComponent();

    void SetUp(
        bool bAdjustPitchBasedOnDistanceToTarget,
        bool bAdjustPitchBasedOnDistanceToTargetUsingCurve
    );

    UPROPERTY(BlueprintAssignable, Category = "Target System | Delegates")
    FOnFinishTargetLock OnFinishTargetLock;

    UPROPERTY(BlueprintAssignable, Category = "Target System | Delegates")
    FOnTargetIsDead OnTargetIsDead;

    UPROPERTY(BlueprintAssignable, Category = "Target System")
    FComponentOnTargetLockedOnOff OnTargetLockedOff;

    UPROPERTY(BlueprintAssignable, Category = "Target System")
    FComponentOnTargetLockedOnOff OnTargetLockedOn;

    UFUNCTION(BlueprintCallable, Category = "Target System")
    bool IsLocked() const;

    UFUNCTION(BlueprintCallable, Category = "Target System")
    AActor* GetLockedOnTargetActor() const;

    UFUNCTION(BlueprintCallable, Category = "Target System")
    virtual void TryStartTargetLock();

    UFUNCTION(BlueprintCallable, Category = "Target System")
    void StopObservingTarget(const bool bIgnoreAutoSwitch = false, const bool bTargetIsDead = false);

    UFUNCTION(BlueprintCallable, Category = "Target System")
    void ControlRotation(bool ShouldControlRotation) const;

    // Enemy switch: step the lock to the adjacent target on the left/right (AxisValue.X sign).
    // Core lives in ExecuteTargetSwitch, reused by SwitchTargetPoint for cross-target overflow.
    UFUNCTION(BlueprintCallable, Category = "Target System")
    virtual void SwitchTarget(FVector2D AxisValue);

    // Unified point scroll: step the lock between the CURRENT target's points (screen-X order, in
    // the AxisValue.X direction). At a target edge it overflows onto the adjacent enemy (via
    // ExecuteTargetSwitch) and lands on that target's entry-edge point. Clamps at the global ends;
    // no wrap. Distinct input from SwitchTarget — bind it to its own action (e.g. scroll).
    UFUNCTION(BlueprintCallable, Category = "Target System")
    virtual void SwitchTargetPoint(FVector2D AxisValue);

#if TARGETSYSTEM_WITH_DEBUG
    // Debug-only read access for the TargetSystemDebug module (GameplayDebugger category +
    // targetsystem.DebugDraw). Everything is captured from the last targeting request — no behaviour.

    // Scored candidates from the last lock-on / auto-switch request (index 0 = winner).
    const TArray<FTargetLockDebugCandidate>& GetDebugLockOnCandidates() const { return DebugLockOnCandidates; }

    // Scored candidates from the last left/right switch request (index 0 = winner).
    const TArray<FTargetLockDebugCandidate>& GetDebugSwitchCandidates() const { return DebugSwitchCandidates; }

    // Mode of the most recent targeting request (lock-on vs switch direction).
    ETargetSwitchMode GetDebugLastMode() const { return DebugLastMode; }

    // The currently locked point on the active target (drives reticle / pitch curve), or null.
    const UTargetPointComponent* GetLockedPoint() const { return LockedPoint; }

    // Post-lock drop range — the target is released once it goes beyond this distance.
    float GetLoseTargetDistance() const { return LoseTargetDistance; }
#endif

protected:
    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    ECharacterRotationMode CharacterRotationMode = ECharacterRotationMode::OrientToMovement;

    // On the current target's death, re-select through the TargetingPreset. The plugin
    // stays dumb: all target-selection / combat gating lives in the preset's tasks. If the preset
    // yields nobody, no re-lock happens and the lock is fully torn down.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    bool bAutoTargetSwitch = true;

    // Minimum horizontal-axis magnitude to trigger a target switch. Switch input is discrete
    // (one-shot ±axis from GA_TargetLock_Select*), so this is a simple gate, not an edge latch.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    float SwitchActivateThreshold = 0.5f;

    // VESTIGIAL for the subsystem path: point-switch eligibility is now configured on the
    // UTargetingTask_SelectTargetPoint::PointQuery inside SwitchPointPreset (the authoritative
    // filter). Kept so existing BP assignments don't break; no longer read by SwitchTargetPoint.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    FTargetPointQuery SwitchPointQuery;

    // Targeting preset (GameplayTargetingSystem) — single source of truth for target selection
    // and switching. SortByLockOn / FilterSwitchTargetSide branch on the request Mode.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem")
    TObjectPtr<UTargetingPreset> TargetingPreset = nullptr;

    // Point-switch preset: runs SelectTargetPoint (+ SortByScreenX) to step the lock between
    // points on the CURRENT target. Isolated from TargetingPreset so point-location baking never
    // leaks into lock-on / actor-switch / re-validation. Until assigned, SwitchTargetPoint no-ops.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem")
    TObjectPtr<UTargetingPreset> SwitchPointPreset = nullptr;

    // Optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Optimization")
    float TimerTick = 0.5f;

    // Distance Settings — LoseTargetDistance drives the post-lock watchdog (drop the target once
    // it goes beyond this range). Acquisition range is governed by the preset's filter tasks.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float LoseTargetDistance = 4000.0f;

    // Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	TSubclassOf<UUserWidget> LockedOnWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	float LockedOnWidgetDrawSize = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	FVector LockedOnWidgetRelativeLocation = FVector(0.0f, 0.0f, 0.0f);

    // Pitch Offset using Curve
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset using Curve")
	bool bAdjustPitchBasedOnDistanceToTargetUsingCurve = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset using Curve")
	UCurveFloat* DefaultPitchOffsetCurve = nullptr;

	// High-ground framing: max extra UPWARD pitch (degrees) added when the target stands above the
	// player, measured from the ground height difference of the two actor origins. 0 disables the
	// high-ground tilt entirely (pure curve framing). A same-level enemy yields ~0 regardless of this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset using Curve", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float HighGroundMaxPitch = 45.0f;

    // Pitch Offset
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset")
	bool bAdjustPitchBasedOnDistanceToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset")
	float PitchDistanceCoefficient = -0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset")
	float PitchDistanceOffset = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset")
	float PitchMin = -50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Pitch Offset")
	float PitchMax = -20.0f;

    // TODO Delete
	UPROPERTY(BlueprintAssignable, Category = "Target System")
	FComponentSetRotation OnTargetSetRotation;

	UPROPERTY()
	TScriptInterface<ITargetSystemInterface> NearestTarget;

	UPROPERTY()
	TArray<TScriptInterface<ITargetSystemInterface>> PotentialTargets;

	// Lock-on point backing the active target (the target's first target point). Drives the
	// per-point pitch curve and the reticle widget attach; null if the target has no points.
	UPROPERTY()
	TObjectPtr<UTargetPointComponent> LockedPoint = nullptr;

	bool bIsSwitchingTarget = false;

protected:
	void StartObservingTarget();
	void MessageFinishTargetLock() const;
	virtual void AutoSwitchTarget();
	void ResetIsSwitchingTarget();

	// Subsystem path: async lock-on result + sync target-switch result callbacks.
	UFUNCTION()
	void OnTargetingCompleted(FTargetingRequestHandle Handle);
	void OnSwitchTargetingCompleted(FTargetingRequestHandle Handle);

	// Sync point-switch completion. The chosen point is read inline from the context after
	// the synchronous request returns, so this is a logging/no-op stub kept for delegate symmetry.
	void OnSwitchPointTargetingCompleted(FTargetingRequestHandle Handle);

private:
	UPROPERTY()
	AActor* OwnerActor = nullptr;

	UPROPERTY()
	APawn* OwnerPawn = nullptr;

	UPROPERTY()
	APlayerController* OwnerPlayerController = nullptr;

	UPROPERTY()
	UWidgetComponent* TargetLockedOnWidgetComponent = nullptr;
	
	bool bTargetLocked = false;

    FTimerHandle SwitchingTargetTimerHandle;
    FTimerHandle ObservingTimer;

#if TARGETSYSTEM_WITH_DEBUG
    // Last scored candidate sets captured from the targeting subsystem for the debug overlays.
    // Index 0 is the winner. Populated in OnTargetingCompleted / OnSwitchTargetingCompleted.
    TArray<FTargetLockDebugCandidate> DebugLockOnCandidates;
    TArray<FTargetLockDebugCandidate> DebugSwitchCandidates;
    ETargetSwitchMode DebugLastMode = ETargetSwitchMode::LockOn;

    // Read (actor, score) pairs from a finished request handle into Out, marking the first valid
    // result as the winner. Cheap: scores are already computed by the sort task and stored on the
    // result data, so nothing is recomputed here.
    void CaptureDebugCandidates(FTargetingRequestHandle Handle, TArray<FTargetLockDebugCandidate>& Out) const;
#endif

    // Enemy-switch core (Mode = SwitchLeft/Right). Returns true if the locked target changed.
    // Shared by SwitchTarget and the SwitchTargetPoint cross-target overflow.
    bool ExecuteTargetSwitch(int32 Direction);

    // Run SwitchPointPreset (SelectTargetPoint) synchronously against TargetActor, stepping from
    // StartPoint in Direction. StartPoint == null => land on the entry-edge point. Returns the
    // chosen point (== StartPoint means we are at the target edge — the boundary contract).
    UTargetPointComponent* RunSwitchPointRequest(AActor* TargetActor, UTargetPointComponent* StartPoint, int32 Direction);

    float GetDistanceFromTarget(const TargetInterface& Interface) const;
    FRotator GetControlRotationOnTarget(TargetInterface Interface) const;
    AActor* GetTargetOwnerActor(const TargetInterface& Interface) const;
    FVector GetTargetOwnerLocation(const TargetInterface& Interface) const;

    // Control-rotation focus: the locked point's world location when a point is locked on the
    // current target, else the actor origin. Lets lock-on aim at head/body/tail.
    FVector GetLockedFocusLocation(const TargetInterface& Interface) const;

    // Pick the lock-on point the player is aiming at: smallest camera angle wins, distance only
    // breaks ties (mirrors SortByLockOn's angle+distance scoring, over a target's points). Used at
    // lock time so the reticle/focus land on the looked-at point instead of GetTargetPoints()[0].
    UTargetPointComponent* SelectBestLockOnPoint(const TArray<UTargetPointComponent*>& Points) const;

    void SetControlRotationOnTarget() const;
    void SetupLocalPlayerController();

    // Pulls valid actors out of a finished targeting request; first valid is returned and
    // all valid are appended to OutTargets.
    AActor* ExtractTargetingResults(FTargetingRequestHandle Handle, TArray<TargetInterface>& OutTargets);

	void CreateAndAttachTargetLockedOnWidgetComponent(const TargetInterface Interface);

    // Re-parent the existing reticle widget to LockedPoint (used by SwitchTargetPoint — avoids
    // recreating the UWidgetComponent on every point switch).
    void MoveReticleToLockedPoint();

    // Lose-target watchdog: fired on ObservingTimer. Drops the lock when the target dies or leaves
    // LoseTargetDistance, then re-selects through the TargetingPreset (auto-switch on death).
    void UpdateTargetInfo();

    void StopTargetLock();
};
