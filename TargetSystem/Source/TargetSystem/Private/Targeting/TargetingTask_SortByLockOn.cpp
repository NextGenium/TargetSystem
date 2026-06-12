// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_SortByLockOn.h"

#include "Targeting/TargetLockContext.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/PlayerCameraManager.h"
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
	const FVector TargetLocation = TargetActor->GetActorLocation();

	const float Distance = FVector::Distance(PlayerPawn->GetActorLocation(), TargetLocation);
	const float DistScore = DistanceScale > 0.f ? Distance / DistanceScale : Distance;

	// Req 1: the real angle between camera forward and the direction to the target. Prefer the
	// camera manager (the lock-on view); fall back to the pawn's eye point/forward if absent.
	FVector ViewLocation;
	FVector ViewForward;
	if (PC->PlayerCameraManager)
	{
		ViewLocation = PC->PlayerCameraManager->GetCameraLocation();
		ViewForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
	}
	else
	{
		ViewLocation = PlayerPawn->GetActorLocation();
		ViewForward = PlayerPawn->GetActorForwardVector();
	}

	const FVector ToTarget = (TargetLocation - ViewLocation).GetSafeNormal();
	const float Dot = FMath::Clamp(FVector::DotProduct(ViewForward.GetSafeNormal(), ToTarget), -1.f, 1.f);
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));
	const float AngleScore = AngleScale > 0.f ? Angle / AngleScale : Angle;

	FVector2D ScreenLoc;
	PC->ProjectWorldLocationToScreen(TargetLocation, ScreenLoc);

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	const FVector2D ScreenCenter = ViewportSize * 0.5f;

	const float PixelOffset = FVector2D::Distance(ScreenLoc, ScreenCenter);
	const float ScreenScore = ScreenOffsetScale > 0.f ? PixelOffset / ScreenOffsetScale : PixelOffset;

	// Lower = better (ascending sort). Angle dominates via AngleWeight > DistanceWeight; distance
	// still contributes; the legacy screen term stays available (ScreenWeight = 0 in the asset).
	return AngleScore * AngleWeight + DistScore * DistanceWeight + ScreenScore * ScreenWeight;
}

float UTargetingTask_SortByLockOn::ComputeSwitchScore(
	const AActor* TargetActor, const APawn* PlayerPawn, APlayerController* PC,
	const UTargetLockContext* TargetLockContext) const
{
	if (TargetLockContext->CurrentTarget == TargetActor)
		return FLT_MAX;

	// Req 3 (360°): rank the surviving (correct-side) candidates by the absolute world-yaw gap from
	// the CURRENT target around the camera — the adjacent neighbour (smallest angular step) wins —
	// with a minor world-distance tiebreak. Screen distance is unreliable for off-screen / behind
	// combatants (ProjectWorldLocationToScreen fails or clamps), so we rank in world space.
	const FVector ViewLocation = IsValid(PC->PlayerCameraManager)
		? PC->PlayerCameraManager->GetCameraLocation()
		: PlayerPawn->GetActorLocation();

	FVector ReferenceDir;
	if (IsValid(TargetLockContext->CurrentTarget))
	{
		ReferenceDir = TargetLockContext->CurrentTarget->GetActorLocation() - ViewLocation;
	}
	else
	{
		ReferenceDir = IsValid(PC->PlayerCameraManager)
			? PC->PlayerCameraManager->GetCameraRotation().Vector()
			: PlayerPawn->GetActorForwardVector();
	}
	ReferenceDir.Z = 0.f;

	FVector CandidateDir = TargetActor->GetActorLocation() - ViewLocation;
	CandidateDir.Z = 0.f;

	const float YawGap = FMath::Abs(FMath::FindDeltaAngleDegrees(
		ReferenceDir.Rotation().Yaw, CandidateDir.Rotation().Yaw));
	const float AngleScore = AngleScale > 0.f ? YawGap / AngleScale : YawGap;

	float Dist = FVector::Distance(PlayerPawn->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceScale > 0.f)
		Dist /= DistanceScale;

	return AngleScore * AngleWeight + Dist * DistanceWeight;
}
