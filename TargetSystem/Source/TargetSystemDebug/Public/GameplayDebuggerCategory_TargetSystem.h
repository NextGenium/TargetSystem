// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

/**
 * Gameplay Debugger category for the TargetSystem plugin.
 *
 * Reads the player pawn's UTargetLockComponent via its public (debug) API and draws:
 *  - lock state and the mode of the last targeting request;
 *  - the ranked candidate sets for the last lock-on and switch requests, each entry showing the
 *    engine score plus the raw camera angle / distance, with the winning candidate highlighted;
 *  - the locked target, a 3D line to it, and the live distance against LoseTargetDistance;
 *  - the target's lock-on points (active / blocked / free) with their PointTags / StateTags.
 */
class FGameplayDebuggerCategory_TargetSystem final : public FGameplayDebuggerCategory
{
public:
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
};

#endif
