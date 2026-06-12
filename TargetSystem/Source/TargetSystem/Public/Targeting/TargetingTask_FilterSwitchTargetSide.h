// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterSwitchTargetSide.generated.h"

/**
 * Filter task used while switching targets: removes candidates that are not on the requested
 * switch side (left / right) of the current locked target. The side is measured by world yaw
 * around the camera, so it stays correct for off-screen or behind-the-back candidates (full 360°).
 * In plain lock-on mode (no switch) the task is transparent and keeps every candidate.
 */
UCLASS(DisplayName = "Filter Switch Target On Wrong Side")
class TARGETSYSTEM_API UTargetingTask_FilterSwitchTargetSide : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
