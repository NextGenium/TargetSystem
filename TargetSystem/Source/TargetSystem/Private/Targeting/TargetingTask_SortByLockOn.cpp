// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_SortByLockOn.h"

#include "Targeting/TargetLockContext.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

float UTargetingTask_SortByLockOn::GetScoreForTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
		return FLT_MAX;

	const UTargetLockContext* TargetLockContext = Cast<UTargetLockContext>(SourceContext->SourceObject);
	const APawn* PlayerPawn = Cast<APawn>(SourceContext->SourceActor);
	if (!PlayerPawn)
		return FLT_MAX;

	APlayerController* PC = Cast<APlayerController>(PlayerPawn->GetController());
	if (!PC)
		return FLT_MAX;

	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!TargetActor)
		return FLT_MAX;

	if (!TargetLockContext || TargetLockContext->Mode == ETargetSwitchMode::LockOn)
	{
		return ComputeLockOnScore(TargetActor, PlayerPawn, PC);
	}

	return ComputeSwitchScore(TargetActor, PlayerPawn, PC, TargetLockContext);
}

float UTargetingTask_SortByLockOn::ComputeLockOnScore(
	const AActor* TargetActor, const APawn* PlayerPawn, APlayerController* PC) const
{
	const float Distance = FVector::Distance(PlayerPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const float DistScore = DistanceScale > 0.f ? Distance / DistanceScale : Distance;

	FVector2D ScreenLoc;
	PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenLoc);

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	const FVector2D ScreenCenter = ViewportSize * 0.5f;

	const float PixelOffset = FVector2D::Distance(ScreenLoc, ScreenCenter);
	const float ScreenScore = ScreenOffsetScale > 0.f ? PixelOffset / ScreenOffsetScale : PixelOffset;

	return ScreenScore * ScreenWeight + DistScore * DistanceWeight;
}

float UTargetingTask_SortByLockOn::ComputeSwitchScore(
	const AActor* TargetActor, const APawn* PlayerPawn, APlayerController* PC,
	const UTargetLockContext* TargetLockContext) const
{
	if (TargetLockContext->CurrentTarget == TargetActor)
		return FLT_MAX;

	// Rank the surviving (correct-side) candidates by how close they sit on screen to the
	// CURRENT target — the adjacent neighbour wins — with a minor world-distance tiebreak.
	FVector2D ReferenceScreen;
	if (IsValid(TargetLockContext->CurrentTarget))
	{
		PC->ProjectWorldLocationToScreen(TargetLockContext->CurrentTarget->GetActorLocation(), ReferenceScreen);
	}
	else
	{
		ReferenceScreen = UWidgetLayoutLibrary::GetViewportSize(PC) * 0.5f;
	}

	FVector2D ScreenPos;
	PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenPos);

	float ScreenGap = FVector2D::Distance(ScreenPos, ReferenceScreen);
	if (ScreenOffsetScale > 0)
		ScreenGap /= ScreenOffsetScale;

	float Dist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceScale > 0)
		Dist /= DistanceScale;

	return ScreenGap * ScreenWeight + Dist * DistanceWeight;
}
