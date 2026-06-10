# Target System Plugin

A cross-platform UE 5.6 C++ plugin that adds a Dark Souls inspired Camera Lock On / Targeting system.

Target selection is driven entirely by the engine's **GameplayTargetingSystem** (`UTargetingPreset` + composable Targeting Tasks) — there is no hand-rolled "find the nearest actor" pipeline. Lock-on points on a target are identified by **Gameplay Tags**, not indices.

Originally a fork of [mklabs/ue4-targetsystemplugin](https://github.com/mklabs/ue4-targetsystemplugin); v2.0 is a ground-up refactor onto GameplayTargetingSystem with tag-driven points, replicated point state, and a Gameplay Debugger category.

## Features

- **GameplayTargetingSystem pipeline** — selection / filter / sort is a `UTargetingPreset` you compose from the shipped `UTargetingTask_*` tasks. No engine-version-locked manual search.
- **Tag-driven lock-on points** — `UTargetPointComponent` carries `PointTags` / `StateTags` / `MetadataTags`. Switch between a boss's head / body / tail by on-screen position, not by fragile integer indices.
- **Runtime point blocking** — `StateTags` are replicated (push-model); add `TargetSystem.Block.Stunned` to a point to temporarily remove it from selection.
- **Source-side filtering** — points can require/block source-actor tags (`RequiredSourceTags` / `BlockedSourceTags`).
- **Single interface** — implement `ITargetSystemInterface` on enemies (`IsTargetable`, `GetTargetPoints`, `QueryTargetPoints`, `OnTargetLockBegin/End`).
- **Gameplay Debugger** — `TargetSystem` category + `targetsystem.DebugDraw` console variable for an in-PIE overlay.
- **Cross-platform.** No `Win64`-only restriction, no bundled demo content.

## Setup

Four steps to get lock-on working in a project:

1. **Enable the plugin.** Drop it in `Plugins/`, enable `TargetSystem` in the editor (the `GameplayTargetingSystem` engine plugin is pulled in as a dependency).
2. **Add the component + a preset.** On the player pawn add a `UTargetSystemComponent` and assign a `TargetingPreset` asset (see the example below).
3. **Make enemies targetable.** Implement `ITargetSystemInterface` on enemy actors and return `IsTargetable() == true`. Optionally add `UTargetPointComponent`s and return them from `GetTargetPoints()` — with no points, lock-on falls back to the actor's location.
4. **Bind input.** Call `Component->TryStartTargetLock()` to lock/unlock and `Component->SwitchTarget(AxisValue)` to switch targets on stick/mouse axis.

That's it — no demo content, mannequin assets, example map, or default input config are required. Those all live on the project side.

### Example targeting preset

A minimal lock-on `UTargetingPreset` is an ordered list of tasks:

| Order | Task | Type | Role |
|---|---|---|---|
| 1 | `UTargetingTask_CollectTargetables` | Selection | Gather actors implementing `ITargetSystemInterface` within `SearchRadius` |
| 2 | `UTargetingTask_FilterByMaxDistance` | Filter | Drop targets past `MaxDistance` |
| 3 | `UTargetingTask_FilterByMaxAngle` | Filter | Keep targets inside a cone from the camera |
| 4 | `UTargetingTask_FilterByLineOfSight` | Filter | Drop targets behind geometry |
| 5 | `UTargetingTask_FilterByViewport` | Filter | Drop targets off-screen |
| 6 | `UTargetingTask_SortByLockOn` | Sort | Rank by screen-offset × `ScreenWeight` + distance × `DistanceWeight` |

The top result becomes the locked target. For switching between targets add a preset using `UTargetingTask_FilterSwitchTargetSide`; for switching between points on one target use `UTargetingTask_SelectTargetPoint` + `UTargetingTask_SortByScreenX`.

### Lock-on points

`UTargetPointComponent` is a `USceneComponent` you attach to an enemy at the spot you want the camera to lock onto. Tag it for semantic identification:

- `PointTags` (design-time, `TargetSystem.Point.*`) — e.g. `Point.Head`, `Point.Body`, `Point.Tail`.
- `StateTags` (runtime, replicated, `TargetSystem.Block.*`) — set/clear via `AddStateTag` / `RemoveStateTag` to block a point.
- `MetadataTags` (design-time, `TargetSystem.Meta.*`) — e.g. `Meta.PreferredForLockOn`.

All native plugin tags live under the `TargetSystem.` prefix; concrete subtags (`Point.Head`, `Block.Stunned`, …) are declared project-side in your `DefaultGameplayTags.ini`.

### Lock-on widget

The lock-on indicator UMG widget is a project-side design-time asset — bind your own spawn/visibility logic to the component's `OnTargetLockedOn` / `OnTargetLockedOff` delegates. No widget asset ships with the plugin.

## Debugging

- **Console variable:** `targetsystem.DebugDraw 1|2|3` — draws the player→target line and target marker (1), lock-on points colored by availability (2), and point tag labels (3). No Gameplay Debugger UI required.
- **Gameplay Debugger:** open the Gameplay Debugger in PIE and enable the **TargetSystem** category to see lock state, the locked target, and each point's `PointTags` / `StateTags` (green = free, red = blocked).

Both are provided by the `TargetSystemDebug` runtime module.

## Thanks and Credits

- To [mklabs](https://github.com/mklabs/ue4-targetsystemplugin) for the original Target System plugin this is forked from.
- To the people over at [Lurendium](http://www.lurendium.com) for the original Dark Souls targeting tutorials the upstream plugin was based on.

## License

MIT License.
