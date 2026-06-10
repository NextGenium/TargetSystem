// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByMaxDistance.generated.h"

/**
 * Filter task that removes targets farther than MaxDistance from the source actor.
 *
 * Replaces the legacy `Distance > MaximumDistanceCanStartTarget` guard (L1).
 */
UCLASS(DisplayName = "Filter By Max Distance")
class TARGETSYSTEM_API UTargetingTask_FilterByMaxDistance : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

public:
	/** Targets beyond this distance from the source actor are removed. <= 0 disables the check. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float MaxDistance = 3000.0f;

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
