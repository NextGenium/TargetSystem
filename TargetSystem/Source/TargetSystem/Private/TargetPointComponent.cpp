// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetPointComponent.h"
#include "TargetPointQuery.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

UTargetPointComponent::UTargetPointComponent()
{
    SetIsReplicatedByDefault(true);

    // Lock-on eligibility runs through MatchesQuery, which rejects inactive points (rule 5). A plain
    // scene component is NOT active by default (bAutoActivate == false => IsActive() == false), so
    // without this every point would be invisible to point-switching. Auto-activate so points are
    // lockable out of the box; runtime disabling still works via Deactivate() or Block.* StateTags.
    bAutoActivate = true;
}

void UTargetPointComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams Params;
    Params.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(UTargetPointComponent, StateTags, Params);
}

bool UTargetPointComponent::MatchesQuery(const FTargetPointQuery& Query) const
{
    // 5) Inactive points never match.
    if (!IsActive())
    {
        return false;
    }

    // 1) Point category must match (empty query matches anything).
    if (!Query.PointTagQuery.IsEmpty() && !Query.PointTagQuery.Matches(PointTags))
    {
        return false;
    }

    // 2) None of the blocker tags may be present on this point.
    if (!Query.StateTagQuery.IsEmpty() && Query.StateTagQuery.Matches(StateTags))
    {
        return false;
    }

    // 3) Source actor must satisfy RequiredSourceTags (empty query matches anything).
    if (!RequiredSourceTags.IsEmpty() && !RequiredSourceTags.Matches(Query.SourceTags))
    {
        return false;
    }

    // 4) Source actor must NOT satisfy BlockedSourceTags.
    if (!BlockedSourceTags.IsEmpty() && BlockedSourceTags.Matches(Query.SourceTags))
    {
        return false;
    }

    return true;
}

void UTargetPointComponent::AddStateTag(FGameplayTag Tag)
{
    if (Tag.IsValid() && !StateTags.HasTagExact(Tag))
    {
        StateTags.AddTag(Tag);
        MARK_PROPERTY_DIRTY_FROM_NAME(UTargetPointComponent, StateTags, this);
    }
}

void UTargetPointComponent::RemoveStateTag(FGameplayTag Tag)
{
    if (StateTags.RemoveTag(Tag))
    {
        MARK_PROPERTY_DIRTY_FROM_NAME(UTargetPointComponent, StateTags, this);
    }
}
