// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByMaxAngle.generated.h"

/**
 * Filter task that removes targets whose yaw offset from the source view direction
 * exceeds MaxAngle (degrees). Uses the source actor's camera orientation when present,
 * otherwise its actor orientation.
 */
UCLASS(DisplayName = "Filter By Max Angle")
class TARGETSYSTEM_API UTargetingTask_FilterByMaxAngle : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

public:
	/** Max absolute yaw offset (degrees) from the view direction. Targets past it are removed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float MaxAngle = 60.0f;

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
