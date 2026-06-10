// Copyright (c) 2024 NextGenium

#include "TargetSystemDebug.h"

#include "GameplayDebuggerCategory_TargetSystem.h"
#include "TargetSystemDebugDraw.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#endif

void FTargetSystemDebugModule::StartupModule()
{
	RegisterGameplayDebuggerCategories();
	TargetSystemDebugDraw::Register();
}

void FTargetSystemDebugModule::ShutdownModule()
{
	TargetSystemDebugDraw::Unregister();
	UnregisterGameplayDebuggerCategories();
}

void FTargetSystemDebugModule::RegisterGameplayDebuggerCategories()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& Debugger = IGameplayDebugger::Get();
		Debugger.RegisterCategory(
			TEXT("TargetSystem"),
			IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_TargetSystem::MakeInstance),
			EGameplayDebuggerCategoryState::EnabledInGameAndSimulate);
		Debugger.NotifyCategoriesChanged();
	}
#endif
}

void FTargetSystemDebugModule::UnregisterGameplayDebuggerCategories()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& Debugger = IGameplayDebugger::Get();
		Debugger.UnregisterCategory(TEXT("TargetSystem"));
		Debugger.NotifyCategoriesChanged();
	}
#endif
}

IMPLEMENT_MODULE(FTargetSystemDebugModule, TargetSystemDebug)
