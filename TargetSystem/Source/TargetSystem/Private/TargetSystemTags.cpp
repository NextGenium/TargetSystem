// Copyright (c) 2024 NextGenium

#include "TargetSystemTags.h"

#include "GameplayTagsManager.h"

namespace TargetSystemTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TargetSystem,        "TargetSystem",        "Root namespace tag owned by the TargetSystem plugin");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TargetSystem_Point,  "TargetSystem.Point",  "Semantic identification of lock-on points (design-time)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TargetSystem_Block,  "TargetSystem.Block",  "Runtime blockers on a point (replicated state-tags)");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TargetSystem_Meta,   "TargetSystem.Meta",   "Design-time metadata on a point");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TargetSystem_Source, "TargetSystem.Source", "Source-actor tags for source-side point filtering");

	FGameplayTagContainer GetAllPointCategories()
	{
		// All registered child tags of TargetSystem.Point — plugin + project-declared.
		return UGameplayTagsManager::Get().RequestGameplayTagChildren(TAG_TargetSystem_Point);
	}

	FGameplayTagContainer GetAllBlockTags()
	{
		// All registered child tags of TargetSystem.Block — plugin + project-declared.
		return UGameplayTagsManager::Get().RequestGameplayTagChildren(TAG_TargetSystem_Block);
	}
}
