// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByLineOfSight.generated.h"

/**
 * Filter task that removes targets obstructed from the source actor by world geometry.
 * Traces a line from the source actor to the target on TraceChannel; the target is
 * removed only when something other than the target actor blocks the line.
 */
UCLASS(DisplayName = "Filter By Line Of Sight")
class TARGETSYSTEM_API UTargetingTask_FilterByLineOfSight : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

public:
	/** Collision channel used for the line-of-sight trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
