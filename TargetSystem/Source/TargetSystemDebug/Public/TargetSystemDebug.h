// Copyright (c) 2024 NextGenium

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Runtime debug module for the TargetSystem plugin. Registers a Gameplay Debugger
 * category (`TargetSystem`) and a `targetsystem.DebugDraw` console variable that draws
 * the lock-on overlay without opening the Gameplay Debugger UI.
 *
 * Mirrors NextBlockActionsDebug.
 */
class FTargetSystemDebugModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	static void RegisterGameplayDebuggerCategories();
	static void UnregisterGameplayDebuggerCategories();
};
