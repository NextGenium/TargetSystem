// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingSortTask.h"
#include "TargetingTask_SortByScreenX.generated.h"

/**
 * Sort task that scores targets by their horizontal screen position (lower X = further
 * left = sorted first). Used to order candidates left-to-right when switching targets.
 *
 * Reads HitResult.Location, so it works on actor results as well as point results
 * produced by UTargetingTask_SelectTargetPoint.
 */
UCLASS(DisplayName = "Sort By Screen X")
class TARGETSYSTEM_API UTargetingTask_SortByScreenX : public USimpleTargetingSortTask
{
	GENERATED_BODY()

	virtual float GetScoreForTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
