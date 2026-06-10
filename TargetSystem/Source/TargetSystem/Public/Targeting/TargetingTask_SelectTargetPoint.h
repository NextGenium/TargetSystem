// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "TargetPointQuery.h"
#include "TargetingTask_SelectTargetPoint.generated.h"

class UTargetPointComponent;

/**
 * Selection task that resolves, for each already-collected target actor, the best
 * lockable UTargetPointComponent matching PointQuery and bakes that point's world
 * location into the result's HitResult (camera / lock-on read HitResult.Location).
 *
 * The point component pointer is stored best-effort only: points are USceneComponent
 * (not UPrimitiveComponent), so HitResult.Component may be null.
 *
 * Per R2, when a target exposes no point matching the query the lock falls back to the
 * actor location (the existing actor result is left unchanged, not removed).
 *
 * Overrides Execute() directly (rather than the USimpleTargetingSelectionTask
 * add-only API) because it must mutate existing results in place; the async state is
 * bracketed manually so the targeting pipeline completes.
 */
UCLASS(DisplayName = "Select Target Point")
class TARGETSYSTEM_API UTargetingTask_SelectTargetPoint : public UTargetingTask
{
	GENERATED_BODY()

public:
	/** Selects which points are lockable (PointTags / StateTags / SourceTags). Empty = all points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	FTargetPointQuery PointQuery;

	virtual void Execute(const FTargetingRequestHandle& TargetingHandle) const override;
};
