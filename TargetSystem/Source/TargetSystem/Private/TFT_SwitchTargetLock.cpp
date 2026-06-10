// Copyright (c) 2024 NextGenium


#include "TFT_SwitchTargetLock.h"

#include "TST_TargetLock.h"
#include "Blueprint/WidgetLayoutLibrary.h"

bool UTFT_SwitchTargetLock::ShouldFilterTarget(
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
	const float InputDirection = TargetLockContext->Mode == ETargetSwitchMode::SwitchLeft ? -1.f : + 1.f;

	FVector2D ScreenPos;
	PC->ProjectWorldLocationToScreen(TargetActor->GetActorLocation(), ScreenPos);

	const float DeltaX = ScreenPos.X - (ViewportSize.X * 0.5f);

	return DeltaX * InputDirection < 0;	
}