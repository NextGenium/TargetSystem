// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_SelectTargetPoint.h"

#include "TargetPointComponent.h"
#include "TargetSystemInterface.h"
#include "Targeting/TargetLockContext.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

namespace
{
	// Gather the actor's eligible lock-on points. The real enemy overrides GetTargetPoints() but
	// not QueryTargetPoints() (interface default {}), so we filter GetTargetPoints() through
	// MatchesQuery here — that honours PointTags / Block.* StateTags / SourceTags (R2).
	TArray<UTargetPointComponent*> GatherEligiblePoints(const ITargetSystemInterface* Interface, const FTargetPointQuery& Query)
	{
		TArray<UTargetPointComponent*> Eligible;
		for (UTargetPointComponent* Point : Interface->GetTargetPoints())
		{
			if (IsValid(Point) && Point->MatchesQuery(Query))
			{
				Eligible.Add(Point);
			}
		}
		return Eligible;
	}
}

void UTargetingTask_SelectTargetPoint::Execute(const FTargetingRequestHandle& TargetingHandle) const
{
	Super::Execute(TargetingHandle);

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Executing);

	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	UTargetLockContext* Ctx = SourceContext ? Cast<UTargetLockContext>(SourceContext->SourceObject) : nullptr;

	// SwitchPoint: step the lock to the adjacent point on the CURRENT target and report it back
	// through Ctx->CurrentPoint (the only viable round-trip channel — see header doc).
	if (Ctx && Ctx->Mode == ETargetSwitchMode::SwitchPoint)
	{
		ExecuteSwitchPoint(TargetingHandle, SourceContext, Ctx);
		SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
		return;
	}

	// LockOn (default): bake the nearest eligible point's location into each actor result.
	ExecuteLockOn(TargetingHandle, SourceContext);

	SetTaskAsyncState(TargetingHandle, ETargetingTaskAsyncState::Completed);
}

void UTargetingTask_SelectTargetPoint::ExecuteLockOn(
	const FTargetingRequestHandle& TargetingHandle, const FTargetingSourceContext* SourceContext) const
{
	FTargetingDefaultResultsSet* ResultsSet = FTargetingDefaultResultsSet::Find(TargetingHandle);
	if (!ResultsSet)
	{
		return;
	}

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

		const TArray<UTargetPointComponent*> Points = GatherEligiblePoints(Interface, PointQuery);
		if (Points.IsEmpty())
		{
			// No point matched the query: lock stays on the actor (fallback to actor location, R2).
			continue;
		}

		UTargetPointComponent* BestPoint = nullptr;
		float BestDistanceSq = TNumericLimits<float>::Max();
		for (UTargetPointComponent* Point : Points)
		{
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
}

void UTargetingTask_SelectTargetPoint::ExecuteSwitchPoint(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingSourceContext* SourceContext,
	UTargetLockContext* Ctx) const
{
	// Resolve the current target: prefer the context's CurrentTarget, fall back to the first result.
	AActor* TargetActor = Ctx->CurrentTarget;
	if (!IsValid(TargetActor))
	{
		if (const FTargetingDefaultResultsSet* ResultsSet = FTargetingDefaultResultsSet::Find(TargetingHandle))
		{
			for (const FTargetingDefaultResultData& TargetData : ResultsSet->TargetResults)
			{
				if (AActor* Actor = TargetData.HitResult.GetActor())
				{
					TargetActor = Actor;
					break;
				}
			}
		}
	}
	if (!IsValid(TargetActor))
	{
		return;
	}

	const ITargetSystemInterface* Interface = Cast<ITargetSystemInterface>(TargetActor);
	if (!Interface)
	{
		return;
	}

	// Candidates: eligible points on the current target (skips blocked/inactive via MatchesQuery).
	TArray<UTargetPointComponent*> Candidates = GatherEligiblePoints(Interface, PointQuery);
	if (Candidates.Num() < 2)
	{
		// Degenerate 0–1 point target: nothing to step to; leave CurrentPoint untouched.
		return;
	}

	// Screen-X sorting needs the player controller behind the source pawn.
	const APawn* SourcePawn = SourceContext ? Cast<APawn>(SourceContext->SourceActor) : nullptr;
	APlayerController* PC = SourcePawn ? Cast<APlayerController>(SourcePawn->GetController()) : nullptr;
	if (!PC)
	{
		return;
	}

	auto ScreenX = [PC](const UTargetPointComponent* Point) -> float
	{
		FVector2D Screen;
		if (!PC->ProjectWorldLocationToScreen(Point->GetComponentLocation(), Screen))
		{
			return TNumericLimits<float>::Max();
		}
		return Screen.X;
	};

	// Sort left-to-right by screen X (off-screen sorts last via FLT_MAX).
	Candidates.Sort([&ScreenX](const UTargetPointComponent& A, const UTargetPointComponent& B)
	{
		return ScreenX(&A) < ScreenX(&B);
	});

	// Locate the current point. If it is no longer eligible (e.g. it just became blocked), use
	// the candidate nearest its last screen X as the reference index.
	int32 CurrentIndex = Candidates.IndexOfByKey(Ctx->CurrentPoint);
	if (CurrentIndex == INDEX_NONE)
	{
		const FVector ReferenceLocation = IsValid(Ctx->CurrentPoint)
			? Ctx->CurrentPoint->GetComponentLocation()
			: TargetActor->GetActorLocation();
		FVector2D ReferenceScreen;
		const float ReferenceX = PC->ProjectWorldLocationToScreen(ReferenceLocation, ReferenceScreen)
			? ReferenceScreen.X
			: 0.f;

		float BestDelta = TNumericLimits<float>::Max();
		for (int32 i = 0; i < Candidates.Num(); ++i)
		{
			const float Delta = FMath::Abs(ScreenX(Candidates[i]) - ReferenceX);
			if (Delta < BestDelta)
			{
				BestDelta = Delta;
				CurrentIndex = i;
			}
		}
	}

	// Step to the adjacent point in the input direction; clamp at the ends (no wrap).
	const int32 Direction = Ctx->SwitchDirection >= 0 ? 1 : -1;
	const int32 NextIndex = FMath::Clamp(CurrentIndex + Direction, 0, Candidates.Num() - 1);

	// The round-trip write: the component reads this back inline after the sync request returns.
	Ctx->CurrentPoint = Candidates[NextIndex];
}
