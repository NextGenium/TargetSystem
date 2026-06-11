// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetSystemInterface.h"
#include "Components/ActorComponent.h"
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

    UFUNCTION(BlueprintCallable, Category = "Target System")
    virtual void SwitchTarget(FVector2D AxisValue);

protected:
    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    ECharacterRotationMode CharacterRotationMode = ECharacterRotationMode::OrientToMovement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    bool bAutoTargetSwitch = false;

    // Minimum horizontal-axis magnitude to trigger a target switch. Switch input is discrete
    // (one-shot ±axis from GA_TargetLock_Select*), so this is a simple gate, not an edge latch.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    float SwitchActivateThreshold = 0.5f;

    // Occlusion channel for the lose-target LOS watchdog (defaults to ECC_Visibility). Set this
    // to whatever channel your walls block; existing BP components may have a stale ECC_Pawn.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    TEnumAsByte<ECollisionChannel> TargetCollisionChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    float BreakLineOfSightDelay = 2.0f;

    // Targeting preset (GameplayTargetingSystem) — single source of truth for target selection
    // and switching. SortByLockOn / FilterSwitchTargetSide branch on the request Mode.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem")
    TObjectPtr<UTargetingPreset> TargetingPreset = nullptr;

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
    FTimerHandle BehindWallTimer;

    float GetDistanceFromTarget(const TargetInterface& Interface) const;
    FRotator GetControlRotationOnTarget(TargetInterface Interface) const;
    AActor* GetTargetOwnerActor(const TargetInterface& Interface) const;
    FVector GetTargetOwnerLocation(const TargetInterface& Interface) const;

    void SetControlRotationOnTarget() const;
    void SetupLocalPlayerController();

    // Pulls valid actors out of a finished targeting request; first valid is returned and
    // all valid are appended to OutTargets.
    AActor* ExtractTargetingResults(FTargetingRequestHandle Handle, TArray<TargetInterface>& OutTargets);

    // Occlusion LOS check for the watchdog: true == clear line of sight to TargetActor (which
    // is ignored, alongside the owner), false == something blocks TargetCollisionChannel.
    bool LineTrace(const FVector& Start, const FVector& End, const AActor* TargetActor, FHitResult& Hit) const;
	void CreateAndAttachTargetLockedOnWidgetComponent(const TargetInterface Interface);

    void UpdateTargetInfo();
    void StopTargetLock();
};
