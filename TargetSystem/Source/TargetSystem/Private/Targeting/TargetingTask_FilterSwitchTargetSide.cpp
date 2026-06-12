// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterSwitchTargetSide.h"

#include "Targeting/TargetLockContext.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterSwitchTargetSide::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext)
	{
		return true;
	}

	const UTargetLockContext* TargetLockContext = Cast<UTargetLockContext>(SourceContext->SourceObject);
	if (!TargetLockContext)
	{
		return true;
	}

	const APawn* PlayerPawn = Cast<APawn>(SourceContext->SourceActor);
	if (!PlayerPawn)
	{
		return true;
	}

	APlayerController* PC = Cast<APlayerController>(PlayerPawn->GetController());
	if (!PC)
	{
		return true;
	}

	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!TargetActor)
	{
		return true;
	}

	if (TargetLockContext->CurrentTarget == TargetActor)
	{
		return true;
	}

	if (TargetLockContext->Mode == ETargetSwitchMode::LockOn)
	{
		return false;
	}

	// Req 3 (360°): side is measured by world yaw around the camera, not screen X — an off-screen or
	// behind-the-back combatant projects to an unreliable / clamped screen position. We compare the
	// horizontal direction (camera -> current target) against (camera -> candidate); the sign of
	// their cross product's Z says which side the candidate is on (+Z = right, -Z = left).
	const float InputDirection = TargetLockContext->Mode == ETargetSwitchMode::SwitchLeft ? -1.f : +1.f;

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
		// No current target: use the camera forward as the reference axis.
		ReferenceDir = IsValid(PC->PlayerCameraManager)
			? PC->PlayerCameraManager->GetCameraRotation().Vector()
			: PlayerPawn->GetActorForwardVector();
	}
	ReferenceDir.Z = 0.f;
	ReferenceDir = ReferenceDir.GetSafeNormal();

	FVector CandidateDir = TargetActor->GetActorLocation() - ViewLocation;
	CandidateDir.Z = 0.f;
	CandidateDir = CandidateDir.GetSafeNormal();

	if (ReferenceDir.IsNearlyZero() || CandidateDir.IsNearlyZero())
	{
		return true;
	}

	// +Z => candidate is to the right of the current target; -Z => to the left.
	const float SideSign = FVector::CrossProduct(ReferenceDir, CandidateDir).Z;

	// Remove anything on the wrong side (and anything angularly aligned with the current target).
	return SideSign * InputDirection <= 0.f;
}
