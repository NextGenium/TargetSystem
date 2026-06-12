// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TargetLockContext.generated.h"

class UTargetPointComponent;

/** What kind of targeting operation a request represents. Carried in the source context. */
UENUM(BlueprintType)
enum class ETargetSwitchMode : uint8
{
	// Initial lock-on: pick the best target.
	LockOn,
	// Switch to another target on the left/right of the current one.
	SwitchLeft,
	SwitchRight,
	// Switch between lock-on points on the current target.
	SwitchPoint
};

/**
 * Per-request context object passed via FTargetingSourceContext::SourceObject.
 * Tasks read it to know the current mode / target / point. Lifecycle = one request.
 */
UCLASS(Blueprintable)
class TARGETSYSTEM_API UTargetLockContext : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Target Lock")
	ETargetSwitchMode Mode = ETargetSwitchMode::LockOn;

	UPROPERTY(BlueprintReadWrite, Category = "Target Lock")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Target Lock")
	TObjectPtr<UTargetPointComponent> CurrentPoint = nullptr;

	// Point-switch only: step direction along the screen-X-sorted point list. -1 = left, +1 = right.
	// SelectTargetPoint reads this in SwitchPoint mode and writes the stepped point back to CurrentPoint.
	UPROPERTY(BlueprintReadWrite, Category = "Target Lock")
	int32 SwitchDirection = 0;
};
