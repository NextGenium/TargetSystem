// Copyright (c) 2024 NextGenium


#include "NextTargetSystemComponent.h"

#include "TST_TargetLock.h"
#include "TargetingSystem/TargetingPreset.h"
#include "TargetingSystem/TargetingSubsystem.h"
#include "Types/TargetingSystemTypes.h"

void UNextTargetSystemComponent::TryStartTargetLock()
{
	if (bUseTargetSubsystem)
	{
		if (!IsValid(TargetingPreset))
		{
			return;
		}
		
		FTargetingSourceContext SourceContext;
		SourceContext.SourceActor = GetOwner();
		UTargetLockContext* TargetLockContext = NewObject<UTargetLockContext>(this);
		SourceContext.SourceObject = TargetLockContext;
		const FTargetingRequestHandle TargetingHandle =
				UTargetingSubsystem::MakeTargetRequestHandle(
					TargetingPreset,
					SourceContext);
		const FTargetingRequestDelegate Delegate = FTargetingRequestDelegate::CreateUObject(
			this,
			&UNextTargetSystemComponent::OnTargetingCompleted);

		UTargetingSubsystem::Get(GetWorld())->StartAsyncTargetingRequestWithHandle(
					TargetingHandle,
					Delegate);
	}
	else
	{
		Super::TryStartTargetLock();
	}
}

void UNextTargetSystemComponent::SwitchTarget(FVector2D AxisValue)
{
	if (bUseTargetSubsystem)
	{
		if (!CanSwitchTarget(AxisValue)) return;
		if (bIsSwitchingTarget) return;
		FTargetingSourceContext SourceContext;
		SourceContext.SourceActor = GetOwner();
		UTargetLockContext* TargetLockContext = NewObject<UTargetLockContext>(this);
		TargetLockContext->CurrentTarget = Cast<AActor>(NearestTarget.GetObject());
		TargetLockContext->Mode = AxisValue.X > 0.f ? ETargetSwitchMode::SwitchRight : ETargetSwitchMode::SwitchLeft;
		SourceContext.SourceObject = TargetLockContext;

		const FTargetingRequestHandle TargetingHandle = UTargetingSubsystem::MakeTargetRequestHandle(TargetingPreset, SourceContext);
		FTargetingRequestDelegate Delegate;
		Delegate.BindLambda([this](const FTargetingRequestHandle& TargetingHandle)
		{
			TArray<AActor*> ActorsToLook;
			UTargetingSubsystem::Get(GetWorld())->GetTargetingResultsActors(TargetingHandle, ActorsToLook);
			for (AActor* Actor : ActorsToLook)
			{
				if (IsValid(Actor))
				{
					bIsSwitchingTarget = true;
					StopObservingTarget();
					NearestTarget = Actor;
					StartObservingTarget();
					ResetIsSwitchingTarget();
					break;
				}
			}
		});
		
		UTargetingSubsystem::Get(GetWorld())->ExecuteTargetingRequestWithHandle(TargetingHandle, Delegate);
	}
	else
	{
		Super::SwitchTarget(AxisValue);
	}
	
}

void UNextTargetSystemComponent::AutoSwitchTarget()
{
	if (bUseTargetSubsystem)
	{
		TryStartTargetLock();
	}
	else
	{
		Super::AutoSwitchTarget();
	}
}

void UNextTargetSystemComponent::OnTargetingCompleted(FTargetingRequestHandle Handle)
{
	TArray<AActor*> ActorsToLook;
	UTargetingSubsystem::Get(GetWorld())->GetTargetingResultsActors(Handle, ActorsToLook);
	for (AActor* Actor : ActorsToLook)
	{
		if (IsValid(Actor))
		{
			NearestTarget = Actor;
			break;
		}
	}
	for (AActor* Actor : ActorsToLook)
	{
		if (IsValid(Actor))
		{
			PotentialTargets.Add(Actor);
		}
	}
	if (!NearestTarget)
	{
		MessageFinishTargetLock();
		return;
	}
	StartObservingTarget();
}