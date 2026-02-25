// Copyright 2018-2021 Mickael Daniel. All Rights Reserved.

#include "TargetSystemInterface.h"

#include "NextTargetSystemComponent.h"

UNextTargetSystemComponent* ITargetSystemInterface::GetTargetSystemComponent_Implementation() const
{
	const AActor* SelfActor = Cast<AActor>(this);
	return IsValid(SelfActor) ? SelfActor->FindComponentByClass<UNextTargetSystemComponent>() : nullptr;
}