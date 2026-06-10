// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "TargetPointQuery.h"
#include "TargetPointComponent.generated.h"

// Scene component placed on a targetable actor marking a lockable point
// (head, body, tail...). Identification is tag-driven (PointTags); runtime
// availability is governed by StateTags and source-side query filtering.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent), BlueprintType)
class TARGETSYSTEM_API UTargetPointComponent final : public USceneComponent
{
    GENERATED_BODY()

public: 
    UTargetPointComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Semantic identification of the point (design-time): Point.Head, Point.Body...
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point", meta = (Categories = "TargetSystem.Point"))
    FGameplayTagContainer PointTags;

    // Runtime blockers, replicated push-model (dirtied via Add/RemoveStateTag): Block.Stunned...
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Target Point", meta = (Categories = "TargetSystem.Block"))
    FGameplayTagContainer StateTags;

    // Design-time metadata: Meta.PreferredForLockOn, Meta.SkipInSwitching...
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point", meta = (Categories = "TargetSystem.Meta"))
    FGameplayTagContainer MetadataTags;

    // Point is available only if the source-actor matches this query (or empty).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point")
    FGameplayTagQuery RequiredSourceTags;

    // Point is NOT available if the source-actor matches this query.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point")
    FGameplayTagQuery BlockedSourceTags;

    // Optional per-point pitch offset curve for camera lock-on (was PitchOffsetCurve).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Target Point")
    TObjectPtr<UCurveFloat> LockOnPitchOffsetCurve = nullptr;

    UCurveFloat* GetLockOnPitchOffsetCurve() const { return LockOnPitchOffsetCurve; }

    // Returns true when this point satisfies the query (see R2 conditions).
    UFUNCTION(BlueprintCallable, Category = "Target Point")
    bool MatchesQuery(const FTargetPointQuery& Query) const;

    UFUNCTION(BlueprintCallable, Category = "Target Point")
    void AddStateTag(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category = "Target Point")
    void RemoveStateTag(FGameplayTag Tag);
};
