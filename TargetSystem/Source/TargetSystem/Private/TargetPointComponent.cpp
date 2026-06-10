// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetPointComponent.h"
#include "TargetPointQuery.h"

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
    if (Tag.IsValid())
    {
        StateTags.AddTag(Tag);
    }
}

void UTargetPointComponent::RemoveStateTag(FGameplayTag Tag)
{
    StateTags.RemoveTag(Tag);
}
