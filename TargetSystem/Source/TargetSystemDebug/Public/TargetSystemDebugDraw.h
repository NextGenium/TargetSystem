// Copyright (c) 2024 NextGenium

#pragma once

// Immediate-mode debug draw driven by the `targetsystem.DebugDraw` console variable.
// Independent of the Gameplay Debugger UI: when the cvar is > 0 a core ticker draws
// the lock-on overlay for every UTargetLockComponent in a Game/PIE world.
//
// Reads the component through its public API only — it never touches component
// internals, so it is unaffected by the component refactor.
namespace TargetSystemDebugDraw
{
	void Register();
	void Unregister();
}
