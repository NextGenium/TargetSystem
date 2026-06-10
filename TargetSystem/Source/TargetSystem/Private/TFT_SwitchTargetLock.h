// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TFT_SwitchTargetLock.generated.h"

/**
 * 
 */
UCLASS(DisplayName="Filter Switch Target On Wrong Side")
class TARGETSYSTEM_API UTFT_SwitchTargetLock : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
