// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterByMaxAngle.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterByMaxAngle::ShouldFilterTarget(
	const FTargetingRequestHandle& TargetingHandle,
	const FTargetingDefaultResultData& TargetData) const
{
	const FTargetingSourceContext* SourceContext = FTargetingSourceContext::Find(TargetingHandle);
	if (!SourceContext || !IsValid(SourceContext->SourceActor))
	{
		return true;
	}

	const AActor* Source = SourceContext->SourceActor;
	const AActor* TargetActor = TargetData.HitResult.GetActor();
	if (!IsValid(TargetActor))
	{
		return true;
	}

	// Prefer the camera orientation; fall back to the actor orientation (legacy parity).
	FVector ViewLocation;
	FRotator ViewRotation;
	if (const UCameraComponent* Camera = Source->FindComponentByClass<UCameraComponent>())
	{
		ViewLocation = Camera->GetComponentLocation();
		ViewRotation = Camera->GetComponentRotation();
	}
	else
	{
		ViewLocation = Source->GetActorLocation();
		ViewRotation = Source->GetActorRotation();
	}

	const FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetActor->GetActorLocation() - ViewLocation).Rotator();
	const float DeltaYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(ViewRotation.Yaw, LookAtRotation.Yaw));

	return DeltaYaw > MaxAngle;
}
