// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByMaxDistance.generated.h"

/**
 * Filter task that removes targets farther than MaxDistance from the source actor. This is the
 * acquisition range cap for lock-on (the post-lock drop range is the component's LoseTargetDistance).
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
