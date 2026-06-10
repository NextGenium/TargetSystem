// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterByViewport.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterByViewport::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return true;
	}

	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!IsValid(TargetActor))
	{
		return true;
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	APlayerController* PC = SourcePawn ? Cast<APlayerController>(SourcePawn->GetController()) : nullptr;
	if (!PC)
	{
		// No player controller: keep the target (legacy IsInViewport returned true).
		return false;
	}

	FVector2D ScreenLocation;
	if (!PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenLocation))
	{
		// Behind the camera / failed projection.
		return true;
	}

	int32 SizeX = 0;
	int32 SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);

	const bool bInside =
		ScreenLocation.X > ScreenMargin &&
		ScreenLocation.Y > ScreenMargin &&
		ScreenLocation.X < static_cast<float>(SizeX) &&
		ScreenLocation.Y < static_cast<float>(SizeY);

	return !bInside;
}
