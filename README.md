# Target System

> **📖 Upstream documentation — mklabs / ue4-targetsystemplugin**
>
> Базовая архитектура (camera lock-on, target selection, line-of-sight break, switch-target по mouse/gamepad axis, lock widget) — в upstream repo и wiki: **https://github.com/mklabs/ue4-targetsystemplugin** (см. [Setup](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Setup), [Configuration](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Configuration), [Blueprint Functions and Events](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Blueprint-Functions-and-Events)). Также доступен на UE Marketplace: [Target System Component](https://www.unrealengine.com/marketplace/en-US/product/target-system-component-plugin).
>
> Этот README описывает плагин на high-level + **NextGenium-доработки** относительно upstream. Для общего описания базового `UTargetSystemComponent` API см. upstream wiki.

## Overview

Camera lock-on / targeting plugin в стиле Dark Souls для action-проектов: actor-компонент `UTargetSystemComponent` находит ближайшего таргет-actor'а в радиусе, удерживает камеру на нём, ломает захват по line-of-sight / distance, переключает таргет по mouse/gamepad axis. Плагин реализован на C++, конфигурируется через UPROPERTY + биндится одной функцией в input.

В Next Framework входит как `3rdParty, Gameplay` плагин. Используется на проекте **Godreaper**. NextGenium-доработка `UNextTargetSystemComponent` добавляет интеграцию с UE **GameplayTargetingSystem** (Targeting Presets) поверх upstream.

## When to use

- Нужен Dark Souls-style camera lock-on для action / souls-like / 3D-боя.
- Нужен готовый component без переписывания target selection / camera control с нуля.
- Хочется переключения целей по mouse/gamepad axis с настраиваемой чувствительностью.
- Нужна интеграция с UE **GameplayTargetingSystem** (TargetingPreset) — используйте NextGenium `UNextTargetSystemComponent` с `bUseTargetSubsystem = true`.

## Boundary

- **Combat / damage / abilities** — плагин не делает; решается через GAS / `NGCombatSystem`.
- **AI поведение цели** — плагин только удерживает таргет; AI враждебности — отдельно.
- **UI lock widget** — базовый widget идёт с upstream, кастомизация — на стороне проекта.
- **Animation IK / hand-targeting** — отдельная зона.

## NextGenium-доработки (поверх upstream)

| PR | Что добавлено |
|---|---|
| [#1](https://github.com/NextGenium/TargetSystem/pull/1) | Support different rotation modes (расширение контроля поворота персонажа при lock-on). |
| [#2](https://github.com/NextGenium/TargetSystem/pull/2) | Ignore viewport — флаг для игнорирования видимости в viewport при target selection. |
| [#3](https://github.com/NextGenium/TargetSystem/pull/3) | `UNextTargetSystemComponent` (subclass of `UTargetSystemComponent`) + интеграция с UE GameplayTargetingSystem (`TargetingPreset` + `FTargetingRequestHandle`). Подменяет встроенный target selection upstream'а на async targeting request через `UTargetingPreset` — opt-in через `bUseTargetSubsystem`. Также `TST_TargetLock` (`UTargetingSelectionTask_*`) и `TFT_SwitchTargetLock` (`UTargetingFilterTask_*`) для использования в Targeting Preset asset'е. |

## Public API (NextGenium-доработки)

| Тип | Класс | Назначение |
|---|---|---|
| Component | `UNextTargetSystemComponent` | NextGenium subclass `UTargetSystemComponent`. Override методов `TryStartTargetLock`, `SwitchTarget`, `AutoSwitchTarget`, `StopObservingTarget` для использования GameplayTargetingSystem. |
| TargetingTask | `UTST_TargetLock` | `UTargetingSelectionTask_*` — добавляет actor'ов в TargetingRequest при использовании в `UTargetingPreset`. |
| TargetingTask | `UTFT_SwitchTargetLock` | `UTargetingFilterTask_*` — фильтрует actor'ов для switch-target. |

Базовый upstream API (`UTargetSystemComponent`, `OnTargetLockedOn/Off` events, `TargetableActors`, `LockedOnWidgetClass`, etc.) — без изменений, см. upstream wiki.

## Modules

| Модуль | Тип | LoadingPhase | Назначение |
|---|---|---|---|
| `TargetSystem` | Runtime | `PreDefault` | Upstream `UTargetSystemComponent` + NextGenium `UNextTargetSystemComponent` + targeting tasks. |

**Plugin dependencies:** `TargetingSystem` (для NextGenium GameplayTargetingSystem integration).

**Platform:** `Win64` (по `.uplugin`).

## Installation

Через Next Framework Loader: **Target System**.

Или вручную:

```bash
cd <YourProject>/Plugins
git clone https://github.com/NextGenium/TargetSystem.git
```

## How to use (NextGenium path)

1. Добавить `UNextTargetSystemComponent` на pawn / character.
2. (опц) Включить `bUseTargetSubsystem = true` + указать `TargetingPreset` (asset типа `UTargetingPreset` с `TST_TargetLock` в selection task'ах).
3. Забиндить input action на `TargetActor()` / `TargetActorWithAxisInput()` — как в upstream.
4. Подписаться на `OnTargetLockedOn` / `OnTargetLockedOff` для own gameplay reaction (анимация, UI индикатор).

Базовый scenario без GameplayTargetingSystem — `bUseTargetSubsystem = false`, всё работает как в upstream.

## TODO

- Carry-over from upstream: см. [issues mklabs/ue4-targetsystemplugin](https://github.com/mklabs/ue4-targetsystemplugin/issues).
- NextGenium-specific: TBD — добавится по мере использования.

## Limitations

- `PlatformAllowList: ["Win64"]` в `.uplugin` — для других платформ нужна явная правка.
- NextGenium GameplayTargetingSystem path требует `TargetingSystem` plugin (UE built-in).
- Базовая привязка к одному "primary" target — для multi-target нужна другая система.

## Origin

Upstream — **`mklabs/ue4-targetsystemplugin`** (https://github.com/mklabs/ue4-targetsystemplugin), MIT License, author **Mickael Daniel** (`<mklabs>`). Изначально разработан как Blueprint туториал на [Lurendium](http://www.lurendium.com), переписан в C++ plugin. Также доступен на UE Marketplace.

NextGenium-форк добавляет 3 PR (rotation modes, ignore viewport, `UNextTargetSystemComponent` с GameplayTargetingSystem integration) — см. секцию «NextGenium-доработки».

## Maintainers

- **[DEV] Daniil Ekimov** (Telegram: `@next_gen_devv`) — support and maintenance в NextGenium-форке.

## References

- **Upstream repo:** https://github.com/mklabs/ue4-targetsystemplugin
- **Upstream wiki:** [Setup](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Setup) · [Configuration](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Configuration) · [Blueprint Functions and Events](https://github.com/mklabs/ue4-targetsystemplugin/wiki/Blueprint-Functions-and-Events)
- **UE Marketplace:** [Target System Component](https://www.unrealengine.com/marketplace/en-US/product/target-system-component-plugin)
- **UE GameplayTargetingSystem:** https://docs.unrealengine.com/5.0/en-US/API/Plugins/TargetingSystem/
- Lurendium tutorial series ([Part 1](http://www.lurendium.com/target-system-similar-to-dark-souls/), [Part 2](http://www.lurendium.com/target-system-similar-dark-souls-blueprint-part-2/), [Part 3](http://www.lurendium.com/target-system-similar-to-dark-souls-blueprint-part-3-final/)).
