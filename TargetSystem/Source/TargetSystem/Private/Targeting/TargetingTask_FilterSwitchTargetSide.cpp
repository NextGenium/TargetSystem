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

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	const float InputDirection = TargetLockContext->Mode == ETargetSwitchMode::SwitchLeft ? -1.f : +1.f;

	FVector2D ScreenPos;
	PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenPos);

	const float DeltaX = ScreenPos.X - (ViewportSize.X * 0.5f);

	return DeltaX * InputDirection < 0;
}
