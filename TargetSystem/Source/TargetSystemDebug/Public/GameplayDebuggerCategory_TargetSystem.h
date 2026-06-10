// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategory.h"

/**
 * Gameplay Debugger category for the TargetSystem plugin.
 *
 * Reads the player pawn's UTargetSystemComponent via its public API (IsLocked /
 * GetLockedOnTargetActor) and draws: lock state, the locked target, a 3D line to it,
 * and the target's lock-on points with their PointTags / StateTags.
 *
 * Candidate/score overlay lights up once UTargetSystemComponent exposes its last
 * targeting result publicly (Step 8 component refactor).
 */
class FGameplayDebuggerCategory_TargetSystem final : public FGameplayDebuggerCategory
{
public:
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
};

#endif
