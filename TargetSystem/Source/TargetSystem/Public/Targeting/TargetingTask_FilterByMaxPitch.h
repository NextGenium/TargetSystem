// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByMaxPitch.generated.h"

/**
 * Filter task that removes targets whose vertical angle (elevation/pitch) from the source
 * view exceeds MaxPitch (degrees). Stops lock-on from grabbing actors high above or far
 * below the player (e.g. an enemy standing on a tall rock) where the camera would tilt
 * steeply up/down. Uses the source actor's camera location when present, otherwise the
 * actor location. The metric is the absolute pitch of the (Target - View) direction, so it
 * is independent of how the camera is currently tilted.
 */
UCLASS(DisplayName = "Filter By Max Pitch")
class TARGETSYSTEM_API UTargetingTask_FilterByMaxPitch : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

public:
	/** Max absolute elevation angle (degrees) from horizontal. Targets past it are removed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float MaxPitch = 45.0f;

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
