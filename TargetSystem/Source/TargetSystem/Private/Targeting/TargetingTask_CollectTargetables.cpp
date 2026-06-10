// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_CollectTargetables.h"

#include "TargetSystemInterface.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

void UTargetingTask_CollectTargetables::SelectTargets_Implementation(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingSourceContext& SourceContext) const
{
	const AActor* Source = SourceContext.SourceActor;
	if (!IsValid(Source))
	{
		return;
	}

	UWorld* World = Source->GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SourceLocation = Source->GetActorLocation();
	const float RadiusSq = SearchRadius * SearchRadius;
	const TSubclassOf<AActor> ActorClass = RequiredActorClass ? RequiredActorClass : TSubclassOf<AActor>(AActor::StaticClass());

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor == Source)
		{
			continue;
		}

		const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(Actor);
		if (!Interface || !Interface->IsTargetable())
		{
			continue;
		}

		if (SearchRadius > 0.f && FVector::DistSquared(SourceLocation, Actor->GetActorLocation()) > RadiusSq)
		{
			continue;
		}

		AddTargetActor(TargetingHandle, Actor);
	}
}
