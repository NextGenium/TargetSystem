// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingSortTask.h"
#include "TargetingTask_SortByLockOn.generated.h"

class APawn;
class APlayerController;

/**
 * Sort task that ranks candidate targets for lock-on and for left/right switching.
 * Lower score = better (the engine sorts ascending and takes the first result).
 *
 * The active behaviour branches on UTargetLockContext::Mode:
 *  - lock-on ranks by a weighted blend of the camera-to-target angle and the world distance;
 *  - switching ranks the neighbours by their world-yaw gap from the current target.
 */
UCLASS(DisplayName = "Target Lock Sort Task")
class TARGETSYSTEM_API UTargetingTask_SortByLockOn : public USimpleTargetingSortTask
{
	GENERATED_BODY()

public:
	// Weight of the angle term (camera forward vs. the direction to the target). The angle weighs
	// MORE than distance, but distance still matters: the default AngleWeight (2) > DistanceWeight (1)
	// makes the most centred enemy win even when a side enemy is somewhat closer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float AngleWeight = 2.0f;

	// Normalisation for the angle term (degrees). Angle/AngleScale keeps the term O(1) so the
	// weights compare against the distance term on the same scale.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float AngleScale = 90.0f;

	// Weight of the horizontal screen-offset term — an older screen-space proxy for the angle.
	// Set to 0 in the preset asset so the real angle term above leads instead.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float ScreenWeight = 1.0f;

	// Weight of the world-distance term. Keeps a much closer target competitive with a centred one.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float DistanceWeight = 1.0f;

	// Normalisation for the distance term (world units): Distance/DistanceScale keeps the term O(1)
	// so it compares against the angle term on the same scale.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float DistanceScale = 5000.0f;

	// Normalisation for the screen-offset term (pixels): Offset/ScreenOffsetScale keeps the term O(1).
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
