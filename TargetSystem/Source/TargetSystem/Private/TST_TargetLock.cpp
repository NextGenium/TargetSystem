// Copyright (c) 2024 NextGenium


#include "TST_TargetLock.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Types/TargetingSystemTypes.h"

float UTST_TargetLock::GetScoreForTarget(
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
	else
	{
		return ComputeSwitchScore(TargetActor, PlayerPawn, PC, TargetLockContext);
	}
}

float UTST_TargetLock::ComputeLockOnScore(
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

float UTST_TargetLock::ComputeSwitchScore(
	const AActor* TargetActor, const APawn* PlayerPawn, APlayerController* PC,
	const UTargetLockContext* TargetLockContext) const
{
	if (TargetLockContext->CurrentTarget == TargetActor)
		return FLT_MAX;

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	FVector2D Center = ViewportSize * 0.5f;
	const float InputDirection = TargetLockContext->Mode == ETargetSwitchMode::SwitchLeft ? -1.f : +1.f;
	Center.X += InputDirection * (ScreenOffsetScale * 0.5f);

	FVector2D ScreenPos;
	PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenPos);
	float ScreenDelta = FMath::Abs(ScreenPos.X - Center.X);
	if (ScreenOffsetScale > 0)
		ScreenDelta /= ScreenOffsetScale;

	float Dist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceScale > 0)
		Dist /= DistanceScale;

	UE_LOG(LogTemp, Warning, TEXT("Target Name: %s, Distance: %f, ScreenDelta: %f, Sum: %f"),
		*TargetActor->GetName(), Dist, ScreenDelta,
		ScreenDelta * ScreenWeight + Dist * DistanceWeight);

	return ScreenDelta * ScreenWeight + Dist * DistanceWeight;
}