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

    virtual void SwitchTarget(FVector2D AxisValue);

protected:
    virtual void BeginPlay() override;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Base params
    UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Target System")
    TSubclassOf<AActor> RequiredClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    ECharacterRotationMode CharacterRotationMode = ECharacterRotationMode::OrientToMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
	bool bIgnoreViewport = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    bool bAutoTargetSwitch = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    float StartRotatingThreshold = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    TEnumAsByte<ECollisionChannel> TargetCollisionChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System")
    float BreakLineOfSightDelay = 2.0f;

    // Target Subsystem (GameplayTargetingSystem). When true, selection/switch run through
    // TargetingPreset; when false, the manual Souls-like search below is used as a fallback.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem",
        meta = (InlineEditConditionToggle))
    bool bUseTargetSubsystem = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Target Subsystem",
        meta = (EditCondition = "bUseTargetSubsystem"))
    TObjectPtr<UTargetingPreset> TargetingPreset = nullptr;

    // Optimization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Optimization")
    float TimerTick = 0.5f;

    // Distance Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float DangerousDistanceToTarget = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
	float MaximumDistanceCanStartTarget = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float LoseTargetDistance = 4000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float MaximumDistanceToPotentialTargets = 2400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float MaximumFindAngle = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Distance Settings")
    float ExtraDistanceToLimitWhenSearchingByAngle = 300.0f;

    // Widget
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	TSubclassOf<UUserWidget> LockedOnWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	float LockedOnWidgetDrawSize = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target System | Widget")
	FString CurrentSocketOnNearestTarget = FString("spine_03");

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

	// Lock-on point backing the active target, when a SelectTargetPoint task baked one into
	// the result. Drives the per-point pitch curve; null with the single lock-on preset.
	UPROPERTY()
	TObjectPtr<UTargetPointComponent> LockedPoint = nullptr;

	bool bIsSwitchingTarget = false;

protected:
	void StartObservingTarget();
	void MessageFinishTargetLock() const;
	virtual void AutoSwitchTarget();
	bool CanSwitchTarget(const FVector2D& AxisValue) const;
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

    bool CanTargetLock() const;
    bool IsInViewport(TargetInterface TargetActor) const;
    bool ObjectIsTargetable(const TargetInterface Interface) const;

    int32 GetPointIndexByName(const FString& Name) const;
    float GetDistanceFromTarget(const TargetInterface& Interface) const;
    float GetAngleUsingCameraRotation(const FVector& Location) const;
    float GetAngleUsingCharacterRotation(const FVector& Location) const;
    FRotator GetControlRotationOnTarget(TargetInterface Interface) const;
    AActor* GetTargetOwnerActor(const TargetInterface& Interface) const;
    FVector GetTargetOwnerLocation(const TargetInterface& Interface) const;

    void SetControlRotationOnTarget() const;
    void SetupLocalPlayerController();

    // Pulls valid actors out of a finished targeting request; first valid is returned,
    // all valid are appended to OutTargets, and LockedPoint is set from the first result.
    AActor* ExtractTargetingResults(FTargetingRequestHandle Handle, TArray<TargetInterface>& OutTargets);

    void AddPotentialTargetsByInterface(const TSubclassOf<AActor>& ActorClass);
    bool LineTrace(const FVector& Start, const FVector& End, FHitResult& Hit) const;
	void CreateAndAttachTargetLockedOnWidgetComponent(const TargetInterface Interface);

    
    void UpdateTargetInfo();
    bool TrySwitchBetweenTargetPoints(FVector2D AxisValue);
    void StopTargetLock();

    void SortPotentialTargetsByDistance(TArray<TScriptInterface<ITargetSystemInterface>>& Array);
    void SortPotentialTargetsByAngle(TArray<TScriptInterface<ITargetSystemInterface>>& Array);

    TargetInterface FindNearestTarget(bool bUseAngle = false);
    TargetInterface FindByHorizontal(TArray<TargetInterface> LookTargets, float AxisValue) const;
    TargetInterface FindByVertical(TArray<TargetInterface> LookTargets, FVector2D AxisValue) const;
    static FRotator FindLookAtRotation(const FVector Start, const FVector Target);
};
