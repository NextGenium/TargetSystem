// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TargetPointQuery.generated.h"

// Query used to filter target points both target-side (PointTags / StateTags)
// and source-side (SourceTags). Mirrors FMotionWarpingPointQuery so both
// plugins share the same query model.
USTRUCT(BlueprintType)
struct TARGETSYSTEM_API FTargetPointQuery
{
    GENERATED_BODY()

    // Point category to match (e.g. {Point.Head OR Point.Body}).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point Query", meta = (Categories = "TargetSystem.Point"))
    FGameplayTagQuery PointTagQuery;

    // Blockers that must NOT be present on the point (e.g. {Block.Stunned}).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point Query", meta = (Categories = "TargetSystem.Block"))
    FGameplayTagQuery StateTagQuery;

    // Source-actor tags used for source-side filtering (Required/Blocked on the point).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point Query")
    FGameplayTagContainer SourceTags;
};
