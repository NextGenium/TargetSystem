// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingFilterTask.h"
#include "TargetingTask_FilterByViewport.generated.h"

/**
 * Filter task that removes targets projected outside the viewport (off-screen or
 * behind the camera). When the source actor has no player controller the target is
 * kept (legacy UTargetSystemComponent::IsInViewport parity).
 */
UCLASS(DisplayName = "Filter By Viewport")
class TARGETSYSTEM_API UTargetingTask_FilterByViewport : public USimpleTargetingFilterTask
{
	GENERATED_BODY()

public:
	/** Inner screen margin (pixels) from the left/top edges (legacy used 10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float ScreenMargin = 10.0f;

	virtual bool ShouldFilterTarget(const FTargetingRequestHandle& TargetingHandle, const FTargetingDefaultResultData& TargetData) const override;
};
