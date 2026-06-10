// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingSortTask.h"
#include "TST_TargetLock.generated.h"

UENUM(BlueprintType)
enum class ETargetSwitchMode : uint8
{
	LockOn,
	SwitchLeft,
	SwitchRight 
};

UCLASS(Blueprintable)
class UTargetLockContext : public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(BlueprintReadWrite)
	ETargetSwitchMode Mode = ETargetSwitchMode::LockOn;

	UPROPERTY(BlueprintReadWrite)
	AActor* CurrentTarget = nullptr;
};


/**
 * 
 */
UCLASS(DisplayName = "Target Lock Sort Task")
class TARGETSYSTEM_API UTST_TargetLock : public USimpleTargetingSortTask
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float ScreenWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float DistanceWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float DistanceScale = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float ScreenOffsetScale = 1200.0f;

protected:
	virtual float GetScoreForTarget(
		const FTargetingRequestHandle& TargetingHandle,
		const FTargetingDefaultResultData& TargetData
	) const override;

private:
	float ComputeLockOnScore(const AActor* Target, const APawn* Pawn, APlayerController* PC) const;
	float ComputeSwitchScore(
		const AActor* Target,
		const APawn* Pawn, APlayerController* PC,
		const UTargetLockContext* TargetLockContext
	) const;
};
