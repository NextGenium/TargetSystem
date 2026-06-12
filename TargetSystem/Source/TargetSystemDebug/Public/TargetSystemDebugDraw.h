// Copyright (c) 2024 NextGenium

#pragma once

// Immediate-mode debug draw driven by the `targetsystem.DebugDraw` console variable.
// Independent of the Gameplay Debugger UI: when the cvar is > 0 a core ticker draws
// the lock-on overlay for every UTargetLockComponent in a Game/PIE world.
//
// Levels (cvar value): 1 = line + target marker; 2 = + lock-on points as camera-facing
// circles; 3 = + point tag labels; 4 = + candidate score circles from the last lock-on
// request (winner green, others orange).
//
// Reads the component through its public (debug) API only — it never touches component
// internals, so it is unaffected by the component refactor.
namespace TargetSystemDebugDraw
{
	void Register();
	void Unregister();
}
