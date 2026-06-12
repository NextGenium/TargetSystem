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

#if ENABLE_DRAW_DEBUG
	// Draw the through-mesh lock overlay (trace + foreground point markers + name/tag labels) for a
	// single component at the given cvar level. Shared by the cvar ticker AND the Gameplay Debugger
	// category, so opening the debugger (apostrophe) reproduces the same x-ray markers with no console
	// command. No-op when the component is null / not locked. Compiled out in shipping.
	void DrawOverlay(class UTargetLockComponent* Component, int32 Level);
#endif
}
