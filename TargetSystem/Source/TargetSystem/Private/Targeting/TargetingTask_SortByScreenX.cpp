// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_SortByScreenX.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

float UTargetingTask_SortByScreenX::GetScoreForTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return TNumericLimits<float>::Max();
	}

	const APawn* SourcePawn = Cast<APawn>(SourceContext->SourceActor);
	APlayerController* PC = SourcePawn ? Cast<APlayerController>(SourcePawn->GetController()) : nullptr;
	if (!PC)
	{
		return TNumericLimits<float>::Max();
	}

	FVector2D ScreenLocation;
	if (!PC->ProjectWorldLocationToScreen(TargetData.HitResult.Location, ScreenLocation))
	{
		return TNumericLimits<float>::Max();
	}

	return ScreenLocation.X;
}
