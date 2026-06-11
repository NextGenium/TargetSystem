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

	// Side is measured relative to the CURRENT target's screen position, not the screen
	// centre — "switch right" means the neighbour to the right of who you are locked on.
	const float InputDirection = TargetLockContext->Mode == ETargetSwitchMode::SwitchLeft ? -1.f : +1.f;

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

	const float DeltaX = ScreenPos.X - ReferenceScreen.X;

	// Remove anything on the wrong side (and anything column-aligned with the current target).
	return DeltaX * InputDirection <= 0.f;
}
