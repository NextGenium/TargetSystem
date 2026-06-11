// Copyright (c) 2024 NextGenium

#include "Targeting/TargetingTask_FilterByMaxPitch.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Actor.h"
#include "Types/TargetingSystemTypes.h"

bool UTargetingTask_FilterByMaxPitch::ShouldFilterTarget(
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

	// Prefer the camera location; fall back to the actor location (parity with FilterByMaxAngle).
	FVector ViewLocation;
	if (const UCameraComponent* Camera = Source->FindComponentByClass<UCameraComponent>())
	{
		ViewLocation = Camera->GetComponentLocation();
	}
	else
	{
		ViewLocation = Source->GetActorLocation();
	}

	// Absolute elevation angle of the direction to the target (0 = horizontal), independent
	// of the current camera tilt.
	const FRotator LookAtRotation = FRotationMatrix::MakeFromX(TargetActor->GetActorLocation() - ViewLocation).Rotator();
	const float ElevationAngle = FMath::Abs(LookAtRotation.Pitch);

	return ElevationAngle > MaxPitch;
}
