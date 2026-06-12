// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/TargetingTask.h"
#include "TargetPointQuery.h"
#include "TargetingTask_SelectTargetPoint.generated.h"

class UTargetPointComponent;
class UTargetLockContext;
struct FTargetingSourceContext;

/**
 * Selection task with two modes, branching on UTargetLockContext::Mode in the source context:
 *
 * - LockOn (default): resolves, for each already-collected target actor, the nearest lockable
 *   UTargetPointComponent matching PointQuery and bakes that point's world location into the
 *   result's HitResult (camera / lock-on read HitResult.Location). The point component pointer
 *   is stored best-effort only: points are USceneComponent (not UPrimitiveComponent), so
 *   HitResult.Component may be null. When a target exposes no matching point the lock falls back
 *   to the actor location (the existing actor result is left unchanged, not removed).
 *
 * - SwitchPoint (head ↔ body ↔ tail): steps the lock from Ctx->CurrentPoint to the adjacent
 *   eligible point on Ctx->CurrentTarget, ordered by screen X, in Ctx->SwitchDirection (clamp,
 *   no wrap), and writes the chosen point back to Ctx->CurrentPoint. This is the only viable
 *   round-trip channel: results are actor-centric (one result per actor), so a single dragon
 *   yields one result and a sort task cannot order points — the stepping must live here.
 *
 * Eligible points are gathered via GetTargetPoints() + MatchesQuery (not QueryTargetPoints,
 * which enemies leave at the interface default {}). PointQuery on this task — configured in the
 * preset asset — is the authoritative eligibility filter.
 *
 * Overrides Execute() directly (rather than the USimpleTargetingSelectionTask add-only API)
 * because it must mutate existing results / the context in place; the async state is bracketed
 * manually so the targeting pipeline completes.
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

private:
	// LockOn: bake the nearest eligible point's location into each actor result.
	void ExecuteLockOn(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext* SourceContext) const;

	// SwitchPoint: step Ctx->CurrentPoint to the adjacent eligible point and write it back.
	void ExecuteSwitchPoint(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext* SourceContext, UTargetLockContext* Ctx) const;
};
