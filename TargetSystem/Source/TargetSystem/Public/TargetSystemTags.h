// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

// Native gameplay tags owned by the TargetSystem plugin.
//
// HARD RULE (see Docs/Requirements.md): every native tag declared by this plugin
// MUST start with the `TargetSystem.` prefix. Only the first-level PREFIXES are
// declared here — concrete subtags (Point.Head, Block.Stunned, Source.Wounded...)
// are project-side, declared in the project's DefaultGameplayTags.ini / Config/Tags.
//
// These prefix tags exist so that `meta=(Categories="TargetSystem.Point")` and
// friends resolve to a registered root in the editor tag selector.
namespace TargetSystemTags
{
	// Root namespace tag. Parent of every TargetSystem.* tag.
	TARGETSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TargetSystem);

	// Semantic identification of lock-on points (design-time): Point.Head, Point.Body...
	TARGETSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TargetSystem_Point);

	// Runtime blockers on a point (replicated state-tags): Block.Stunned, Block.Shield...
	TARGETSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TargetSystem_Block);

	// Design-time metadata on a point: Meta.PreferredForLockOn, Meta.NoAutoSwitch...
	TARGETSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TargetSystem_Meta);

	// Source-actor tags consumed for source-side point filtering: Source.Wounded...
	TARGETSYSTEM_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_TargetSystem_Source);

	// All registered child point categories (TargetSystem.Point.*) — plugin + project.
	TARGETSYSTEM_API FGameplayTagContainer GetAllPointCategories();

	// All registered child blockers (TargetSystem.Block.*) — plugin + project.
	TARGETSYSTEM_API FGameplayTagContainer GetAllBlockTags();
}
