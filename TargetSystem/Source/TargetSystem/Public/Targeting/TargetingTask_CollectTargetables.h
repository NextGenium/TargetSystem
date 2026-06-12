// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Tasks/SimpleTargetingSelectionTask.h"
#include "TargetingTask_CollectTargetables.generated.h"

/**
 * Selection task that collects every actor implementing ITargetSystemInterface with
 * IsTargetable() == true within SearchRadius of the source actor. This is the entry task
 * of a lock-on preset — it seeds the candidate set that the filter and sort tasks refine.
 */
UCLASS(DisplayName = "Collect Targetable Actors")
class TARGETSYSTEM_API UTargetingTask_CollectTargetables : public USimpleTargetingSelectionTask
{
	GENERATED_BODY()

public:
	/** Max distance from the source actor at which actors are collected. <= 0 means unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	float SearchRadius = 5000.0f;

	/** Only actors of (or derived from) this class are considered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	TSubclassOf<AActor> RequiredActorClass = AActor::StaticClass();

	/**
	 * Require at least one UTargetPointComponent to be lockable: actors exposing no target points
	 * are skipped entirely (they never enter the candidate set, so neither lock-on nor target-switch
	 * can pick them). Default true. Set false on the preset asset to allow point-less actor locks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Targeting")
	bool bRequireTargetPoint = true;

	virtual void SelectTargets_Implementation(const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext& SourceContext) const override;
};
