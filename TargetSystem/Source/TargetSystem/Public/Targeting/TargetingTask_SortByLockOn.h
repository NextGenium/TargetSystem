// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingSortTask.h"
#include "TargetingTask_SortByLockOn.generated.h"

class APawn;
class APlayerController;

/**
 * Sort task that ranks targets for lock-on / switch by a weighted blend of
 * screen-offset (distance from screen center / input side) and world distance.
 *
 * Renamed from UTST_TargetLock (v1.x). Behaviour preserved.
 */
UCLASS(DisplayName = "Target Lock Sort Task")
class TARGETSYSTEM_API UTargetingTask_SortByLockOn : public USimpleTargetingSortTask
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
		const class UTargetLockContext* TargetLockContext
	) const;
};
