// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_SelectTargetPoint.h"

#include "TargetPointComponent.h"
#include "TargetSystemInterface.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

void UTargetingTask_SelectTargetPoint::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	FTargetingDefaultResultsSet* ResultsSet = FTargetingDefaultResultsSet::Find(TargetingHandle);
	if (!ResultsSet)
	{
		SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
		return;
	}

	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	const FVector SourceLocation = (SourceContext && IsValid(SourceContext->SourceActor))
		? SourceContext->SourceActor->GetActorLocation()
		: FVector::ZeroVector;

	for (FTargetingDefaultResultData& TargetData : ResultsSet->TargetResults)
	{
		AActor* TargetActor = TargetData.HitResult.GetActor();
		if (!IsValid(TargetActor))
		{
			continue;
		}

		const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(TargetActor);
		if (!Interface)
		{
			// No interface: keep the actor result as-is (fallback to actor location, R2).
			continue;
		}

		const TArray<UTargetPointComponent*> Points = Interface->QueryTargetPoints(PointQuery);
		if (Points.IsEmpty())
		{
			// No point matched the query: lock stays on the actor (fallback to actor location, R2).
			continue;
		}

		UTargetPointComponent* BestPoint = nullptr;
		float BestDistanceSq = TNumericLimits<float>::Max();
		for (UTargetPointComponent* Point : Points)
		{
			if (!IsValid(Point))
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(SourceLocation, Point->GetComponentLocation());
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestPoint = Point;
			}
		}

		if (!IsValid(BestPoint))
		{
			continue;
		}

		// Bake the chosen point's world location into the result. The camera / lock-on
		// reads HitResult.Location; the component pointer is best-effort (points are
		// USceneComponent, not UPrimitiveComponent, so the cast may yield null).
		const FVector PointLocation = BestPoint->GetComponentLocation();
		TargetData.HitResult.Location = PointLocation;
		TargetData.HitResult.ImpactPoint = PointLocation;
		TargetData.HitResult.Component = Cast<UPrimitiveComponent>(BestPoint);
	}

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}
