## 2.0.0 — v2.0 refactor (UE 5.6)

Ground-up refactor from "upstream mklabs + a `Next*` layer on top" into a cohesive,
cross-platform targeting plugin built on the engine's **GameplayTargetingSystem**.
Backward compatibility is preserved through `CoreRedirects` where cheap; API is broken
where it was the right call. See `Docs/Requirements.md` for the full rationale.

### Breaking Changes

- **Lock-on points are tag-driven.** `UBTargetPoint` (`BTargetPoint`) is replaced by
  `UTargetPointComponent`. The `Index` (int32) and `StartTargetPointName` (FString)
  fields are gone — points are now identified by `PointTags` and ordered by screen-space
  sorting. `CoreRedirect` maps the old class to the new one.
- **Single targetable interface.** `ITargetSystemTargetableInterface` and
  `ITargetSystemOwnerInterface` are removed and merged into `ITargetSystemInterface`,
  which gains `QueryTargetPoints(FTargetPointQuery)` and `OnTargetLockBegin/End`.
  `CoreRedirect` maps the old `TargetableInterface`.
- **Removed `UTargetSystemDependencies` + `FTargetActorDetails`.** Target points are
  queried directly through `ITargetSystemInterface::GetTargetPoints()` /
  `QueryTargetPoints()`.
- **Targeting Task renames** (UE `UTargetingTask_*` convention, `CoreRedirect`s provided):
  - `TST_TargetLock` → `UTargetingTask_SortByLockOn`
  - `TFT_SwitchTargetLock` → `UTargetingTask_FilterSwitchTargetSide`
- **Bundled `Content/` removed**; `CanContainContent` is now `false`. The lock-on widget
  is a project-side design-time asset (bind to `OnTargetLockedOn` / `OnTargetLockedOff`).
- **Cross-platform.** `PlatformAllowList` (`Win64`-only) removed.
- **`Config/DefaultInput.ini` removed** — projects use Enhanced Input.

### Added

- **GameplayTargetingSystem pipeline.** Target selection/filter/sort is composed from
  shipped `UTargetingTask_*` tasks inside a `UTargetingPreset`:
  `CollectTargetables`, `FilterByMaxDistance`, `FilterByMaxAngle`, `FilterByLineOfSight`,
  `FilterByViewport`, `SortByLockOn`, `FilterSwitchTargetSide`, `SelectTargetPoint`,
  `SortByScreenX`.
- **`FTargetPointQuery` + `MatchesQuery`** — target-side (`PointTags`/`StateTags`) and
  source-side (`RequiredSourceTags`/`BlockedSourceTags`) point filtering.
- **Replicated `StateTags`** (push-model) on `UTargetPointComponent` for runtime point
  blocking (e.g. `TargetSystem.Block.Stunned`).
- **Native gameplay tag prefixes** owned by the plugin: `TargetSystem.Point.*`,
  `TargetSystem.Block.*`, `TargetSystem.Meta.*`, `TargetSystem.Source.*` (prefixes only;
  concrete subtags are declared project-side).
- **`TargetSystemDebug` module** — a Gameplay Debugger `TargetSystem` category plus a
  `targetsystem.DebugDraw` (0/1/2/3) console variable for an in-PIE lock-on overlay.

### Changed

- Engine target updated to **UE 5.6**.
- `PitchOffsetCurve` on the point renamed to `LockOnPitchOffsetCurve` (`CoreRedirect`).
- `CoreRedirects` in `Config/DefaultTargetSystem.ini` consolidated and de-duplicated.

---

#### 1.27.0+5.0 (2022-04-09)

* ue5 release

#### 1.27.0 (2021-09-12)

##### New Features

*  Adding support for Local multiplayer Split Screen ([b018bd64](https://github.com/mklabs/ue4-targetsystemplugin/commit/b018bd64778f54b539fd124fb634b78e34742367))

##### Code Style Changes

* **TargetSystemComponent:**  Fixup resharper warnings / typos ([eab6cc0f](https://github.com/mklabs/ue4-targetsystemplugin/commit/eab6cc0f67336b08f33511907034f4138e262aa5))
