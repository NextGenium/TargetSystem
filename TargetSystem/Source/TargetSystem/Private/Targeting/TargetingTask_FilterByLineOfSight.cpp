// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterByLineOfSight.h"

#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterByLineOfSight::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext || !IsValid(SourceContext->SourceActor))
	{
		return true;
	}

	AActor* Source = SourceContext->SourceActor;
	AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!IsValid(TargetActor))
	{
		return true;
	}

	const UWorld* World = Source->GetWorld();
	if (!World)
	{
		return true;
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Source);
	for (AActor* ChildActor : Source->Children)
	{
		Params.AddIgnoredActor(ChildActor);
	}

	FHitResult Hit;
	const bool bBlocked = World->LineTraceSingleByChannel(
		Hit,
		Source->GetActorLocation(),
		TargetActor->GetActorLocation(),
		TraceChannel,
		Params);

	// Removed only when an obstruction (something other than the target) blocks the line.
	return bBlocked && Hit.GetActor() != TargetActor;
}
