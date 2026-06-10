// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterSwitchTargetSide.generated.h"

/**
 * Filter task that removes targets which are not on the requested switch side
 * (left / right of screen center) relative to the current locked target.
 *
 * Renamed from UTFT_SwitchTargetLock (v1.x). Behaviour preserved.
 */
UCLASS(DisplayName = "Filter Switch Target On Wrong Side")
class TARGETSYSTEM_API UTargetingTask_FilterSwitchTargetSide : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
