// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterByMaxDistance.h"

#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterByMaxDistance::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	if (MaxDistance <= 0.f)
	{
		return false;
	}

	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext || !IsValid(SourceContext->SourceActor))
	{
		return true;
	}

	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!IsValid(TargetActor))
	{
		return true;
	}

	return SourceContext->SourceActor->GetDistanceTo(TargetActor) > MaxDistance;
}
