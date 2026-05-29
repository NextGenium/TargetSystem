# TargetSystem v2.0 — требования к рефакторингу

Статус: **draft / requirements**. Перед началом имплементации согласовать с командой и закрыть открытые вопросы из последнего раздела.

Плагин: [Plugins/TargetSystem/](../) (форк `mklabs/ue4-targetsystemplugin` + NextGenium-доработки 2024–2025).

Эта работа — **v2.0 рефакторинг**, не патч. Backward-compat пытаемся сохранить через `CoreRedirects` где это дёшево, но API ломаем там, где это правильно.

---

## Цель

Превратить плагин из «upstream mklabs + слой `Next*` сверху» в **cohesive cross-platform targeting plugin**, у которого:

1. **Единственный источник правды для выбора целей и точек** — `GameplayTargetingSystem` (UE Plugin). Никаких параллельных «найди ближайшего вручную» pipeline'ов.
2. **Чистая иерархия классов** — один компонент-владелец, один компонент-точка, один интерфейс targetable, набор composable Targeting Task'ов.
3. **Tag-driven identification** точек (по образцу `NextMotionWarping`), отказ от ручных индексов.
4. **Простой setup** в проект: 1 компонент на игрока + интерфейс на врагах + опциональные точки + Targeting Preset.
5. **Кросс-платформенность.** Никакого `Win64`-only.
6. **Чистый репозиторий.** Drop `Content/` целиком.

---

## Текущее состояние

### Файловая инвентаризация ([Source/TargetSystem/](../TargetSystem/Source/TargetSystem/))

| Файл | LOC | Категория | Что делать |
|---|---:|---|---|
| `Public/TargetSystemComponent.h` + `Private/TargetSystemComponent.cpp` | 1023 | Upstream (mklabs), Souls-like camera lock + target search + widget + switching | **Split**: камера/виджет/lock state остаются, ручной target search → удаляется в пользу GameplayTargetingSystem |
| `Public/NextTargetSystemComponent.h` + `Private/NextTargetSystemComponent.cpp` | 184 | NextGenium subclass с `bUseTargetSubsystem` opt-in | **Merge** в `UTargetSystemComponent`, opt-in становится дефолтом (always-on) |
| `Public/BTargetPoint.h` + `Private/BTargetPoint.cpp` | 28 | Точка на цели, есть `Index`, `PitchOffsetCurve` | **Rename** → `UTargetPointComponent`, убрать `Index`, добавить `PointTags`/`StateTags`/`MetadataTags` |
| `Public/TargetSystemInterface.h` | 33 | Targetable interface (Blueprintable) | **Merge** с `TargetSystemTargetableInterface`, убрать `OwnerInterface` |
| `Public/TargetSystemTargetableInterface.h` | 27 | Дубликат метода `IsTargetable` | **Удалить**, слить в `ITargetSystemInterface` |
| `Public/TargetSystemOwnerInterface.h` | 32 | Marked TODO «удалить» | **Удалить полностью** |
| `Public/TargetSystemDependencies.h` + `Private/TargetSystemDependencies.cpp` | 71 | Wrapper-компонент над `FTargetActorDetails` | **Удалить**, точки запрашиваются напрямую через интерфейс |
| `Public/TargetActorDetails.h` | 25 | Struct с `TargetPoints` + `StartTargetPointName` (FString!) | **Удалить**, заменить query-API на интерфейсе |
| `Public/TST_TargetLock.h` + `Private/TST_TargetLock.cpp` | 149 | Sort task `USimpleTargetingSortTask` со ScreenWeight/DistanceWeight | **Сохранить, переименовать** → `UTargetingTask_SortByLockOn` (по конвенции `UTargetingTask_*`) |
| `Private/TFT_SwitchTargetLock.h` + `Public/TFT_SwitchTargetLock.cpp` | 19 | Filter task для switching. **СВАПНУТО** Public/Private | **Сохранить, переименовать** → `UTargetingFilterTask_SwitchTargetSide`, исправить расположение |
| `Public/OverrideCameraDistanceVolume.h` + `Private/.cpp` | 290 | Volume с timeline curve для смены spring arm distance | **Открытый вопрос Q1** — выкинуть в проект Bogatyr или оставить как side-feature |
| `Public/TargetSystem.h` + `Private/TargetSystem.cpp` | 37 | Module bootstrap | Оставить, добавить debug-module |
| `Public/TargetSystemLog.h` + `Private/TargetSystemLog.cpp` | 17 | Log category | Оставить |
| `Private/TargetSystemInterface.cpp`, `TargetSystemOwnerInterface.cpp`, `TargetSystemTargetableInterface.cpp` | 30 | Generated stubs | Удалятся вместе с .h |

**Итого:** ~1940 LOC → цель **~1100-1300 LOC** после рефакторинга.

### Content (48 файлов) — drop полностью

```
Content/Data/CF_TargetLockPitchDistance.uasset       # Curve - вынести в README пример
Content/Example/BP_TargetSystem*                      # mklabs demo - drop
Content/Example/Geometry/Meshes/*                     # mklabs demo cubes - drop
Content/Example/Mannequin/**                          # Epic Mannequin - drop
Content/Example/ThirdPerson/Meshes/*                  # mklabs demo - drop
Content/Maps/*                                        # mklabs demo maps - drop
Content/UI/Assets/LockOnIcon.uasset                   # икона lock-on - вынести в проект если нужна
Content/UI/WBP_LockOn.uasset                          # widget lock-on - вынести в проект
```

После drop'а:
- В README раздел «Setup» документирует, что lock-on widget — design-time asset на стороне проекта (template UMG class пример в README inline).
- `CanContainContent` в `.uplugin` → `false`.

### Config

- `Config/DefaultTargetSystem.ini` — оставить только `[CoreRedirects]` секцию (для backward-compat), консолидировать дубликаты. Удалить `[/Script/Engine.InputSettings]` блок (legacy mappings, проектная зона).
- `Config/DefaultInput.ini` — **дропнуть** (Action/Axis mappings 2018-era, Enhanced Input давно дефолт).
- `Config/FilterPlugin.ini` — оставить пустым.

### .uplugin (важные изменения)

```diff
- "VersionName": "1.28.0",
+ "VersionName": "2.0.0",
- "PlatformAllowList": ["Win64"]
+ // удалить — кросс-платформа
- "CanContainContent": true,
+ "CanContainContent": false,
+ // добавить debug module
```

---

## Что считается легаси

| # | Легаси | Причина |
|---|---|---|
| L1 | Ручной target search в `UTargetSystemComponent` (`FindNearestTarget`, `SortPotentialTargetsByDistance`, `SortPotentialTargetsByAngle`, `AddPotentialTargetsByInterface`) | Дублирует `GameplayTargetingSystem`. v2.0 — единственный pipeline через TargetingPreset |
| L2 | `bUseTargetSubsystem` opt-in flag | В v2.0 GameplayTargetingSystem **всегда on**. Old pipeline удаляется |
| L3 | `BTargetPoint::Index` (int32) + `StartTargetPointName` (FString!) | Хрупкое, не масштабируется, не блюпринтерское. Заменяется на `PointTags` + sort tasks (см. раздел про индексы) |
| L4 | `ITargetSystemTargetableInterface` (отдельный интерфейс на `IsTargetable`) | Дубликат метода из `ITargetSystemInterface` |
| L5 | `ITargetSystemOwnerInterface` | Помечен TODO «удалить» автором, никакой ясной роли |
| L6 | `UTargetSystemDependencies` + `FTargetActorDetails` | Лишняя обёртка-компонент над данными точек. Точки запрашиваются напрямую |
| L7 | `B`-префикс (`UBTargetPoint`) | Реликт Bogatyr-неймконвенции, в плагине неуместен |
| L8 | `CurrentSocketOnNearestTarget : FString` | `FString` для имени компонента в hot path. Минимум `FName`, лучше `FGameplayTag` |
| L9 | Свапнутые Public/Private у `TFT_SwitchTargetLock` (`.cpp` в Public, `.h` в Private) | Просто баг, исправить при рефакторинге |
| L10 | `PlatformAllowList: ["Win64"]` | Никаких причин держать. Кросс-платформа |
| L11 | `Config/DefaultInput.ini` со старыми Action/AxisMappings | Enhanced Input давно дефолт |
| L12 | `Content/` 48 файлов | User explicitly drop. Mklabs demo + Mannequin |
| L13 | Префиксы task'ов `TST_*`, `TFT_*` | UE-конвенция — `UTargetingTask_*`. Переименовать с CoreRedirect |
| L14 | Отсутствие GameplayDebuggerCategory | Дебаг target lock'а сейчас только через `UE_LOG`. Без интерактивного overlay |
| L15 | UMG/Slate в `PrivateDependencyModuleNames` | UMG — да, нужен для widget; Slate/SlateCore — посмотреть, нужны ли они вообще после удаления legacy виджета |

---

## Решение по индексам (Q3 пользователя)

> «раньше точки имели индексы теперь от них возможно стоит отказаться в пользу выбора через GameplayTargetingSystem но я не уверен в этом решении отговори если оно плохое»

**Решение: убрать индексы, заменить тегами + spatial sort.**

### Почему сейчас есть индексы

В upstream `Index` — это **ручной порядок навигации** между точками на одной цели. Switching по стику ([TargetSystemComponent.cpp:279 `TrySwitchBetweenTargetPoints`](../TargetSystem/Source/TargetSystem/Private/TargetSystemComponent.cpp#L279)): берёт текущий index, +1 или -1 по знаку оси, грузит точку с новым индексом.

### Почему ручной индекс плохо

- **Хрупкость:** добавил точку в середину → все последующие индексы поехали, сейвы и blueprint'ы ломаются.
- **Не отражает экранную геометрию:** designer кладёт точки головы/ноги/хвоста — индекс ≠ экранному порядку. Игрок тянет стик влево, ожидает точку слева на экране, получает «следующую по индексу».
- **Не масштабируется:** на боссе с 6 точками держать в голове `0=голова, 1=правая нога, 2=хвост...` — пытка.
- **Не блюпринтерское:** дизайнеру ручная нумерация — не интуитивно.

### Чем заменяем (без потери функциональности)

1. **`PointTags`** на `UTargetPointComponent` — семантическая идентификация (`TargetSystem.Point.Head`, `Body`, `Tail`, `Leg`).
2. **`UTargetingTask_SortByScreenX`** — sort task, который ранжирует точки по экранной X-координате. **Switching по стику = next/prev в этом отсортированном списке.** Стик влево → точка слева на экране. Естественно.
3. **«Стартовая точка»** при первичном lock — через query в TargetingPreset: «приоритет точкам с тегом `Point.Body`», fallback на остальные. Не требует ручного `StartTargetPointName`.
4. **`StateTags`** (replicated) — для runtime-блокировки точек, как в MotionWarping. Если на голове дракона тег `Block.Stunned` — её можно временно убрать из выборки.

### Что НЕ теряем

- **Дизайнерский контроль** — теги дают семантический контроль более выразительный, чем числа.
- **Предсказуемость** — стик-влево всегда даёт точку слева на экране (на самом деле это **более** предсказуемо, чем «следующий index», порядок которого designer мог расставить как угодно).
- **Стабильность сейвов** — `StartTargetPointName: FString` всё равно строка и тоже ломалась при rename'е компонента. `FGameplayTag` + CoreRedirects решает это надёжнее.

### Что теряем (и почему не критично)

- Нельзя сказать «у этого босса жёсткий порядок: ВСЕГДА сначала голова, потом тело, потом хвост — независимо от экрана». Если такой кейс есть — **добавляется опциональный** `UTargetingTask_SortByTagOrder` (массив тегов, сортирует по позиции тега в массиве). Это **отдельный sort task**, не дефолт. Я бы не закладывал его в v2.0 пока не появится конкретный designer-кейс.

---

## Требования

### R1. Точки с tag-driven идентификацией

- `UTargetPointComponent : USceneComponent` — заменяет `UBTargetPoint`.
- `PointTags : FGameplayTagContainer` (design-time, `meta=(Categories="TargetSystem.Point")`) — семантика точки (`Point.Head`, `Point.Body`...).
- `StateTags : FGameplayTagContainer` (runtime, **реплицируется push-model**, `meta=(Categories="TargetSystem.Block")`) — для блокировки.
- `MetadataTags : FGameplayTagContainer` (design-time, `meta=(Categories="TargetSystem.Meta")`) — например `Meta.PreferredForLockOn`, `Meta.SkipInSwitching`.
- `RequiredSourceTags : FGameplayTagQuery` (design-time) — точка доступна только если source-actor matches этот query (см. R2).
- `BlockedSourceTags : FGameplayTagQuery` (design-time) — точка НЕ доступна если source-actor matches этот query.
- `LockOnPitchOffsetCurve : UCurveFloat` — сохранить (переименован из `PitchOffsetCurve`).
- **Никаких `Index`**, **никаких `StartTargetPointName`**.
- API: `AddStateTag` / `RemoveStateTag` / `MatchesQuery(const FTargetPointQuery& Query)`.

### R2. Query mechanism (target-side и source-side фильтрация)

Аналог `R2` в [NextMotionWarping/Requirements.md](../../NextMotionWarping/Docs/Requirements.md) — один и тот же паттерн query, копируем 1:1 чтобы оба плагина выглядели единообразно.

**Структура запроса:**

```cpp
USTRUCT(BlueprintType)
struct FTargetPointQuery
{
    GENERATED_BODY()

    // Категория точки — что подходит (напр. {Point.Head OR Point.Body})
    UPROPERTY(EditAnywhere, meta=(Categories="TargetSystem.Point"))
    FGameplayTagQuery PointTagQuery;

    // Блокеры — что не должно висеть (напр. {Block.Stunned})
    UPROPERTY(EditAnywhere, meta=(Categories="TargetSystem.Block"))
    FGameplayTagQuery StateTagQuery;

    // Теги source-actor'а для source-side фильтра
    UPROPERTY(EditAnywhere)
    FGameplayTagContainer SourceTags;
};
```

**`UTargetPointComponent::MatchesQuery(Query)`** возвращает `true` когда:

1. `Query.PointTagQuery` matches `PointTags` (или query пустой);
2. `Query.StateTagQuery` НЕ matches `StateTags` (т.е. блокеров нет);
3. `RequiredSourceTags` matches `Query.SourceTags` (или query пустой);
4. `BlockedSourceTags` НЕ matches `Query.SourceTags`;
5. Компонент `IsActive()`.

**Кто потребители query:**

- `UTargetingTask_SelectTargetPoint` — основной consumer внутри `UTargetingPreset` (см. R5).
- `UTargetSystemComponent` напрямую при первичном выборе точки на lock-on и при switching между точками.
- Project-side ability'и при нестандартных сценариях.

**Поведение при пустом результате фильтра:**

> Решено — **fallback на actor location**. Если ни одна точка не прошла query, lock-on продолжается **на сам actor** (через `GetActorLocation()`), не отменяется. Это отличается от `NextMotionWarping` R2 (где варп skip'ался) — потому что у lock-on нет «варп skip» состояния: если игрок нажал lock, ему нужно что-то залочить.

Если **сам actor** недоступен (`IsTargetable() = false`) — это обрабатывается `UTargetingTask_CollectTargetables`, до query на точки дело не дойдёт.

**Сценарии (см. также Use cases ниже):**

- **Stunned-голова дракона:** ability stun → `HeadPoint->AddStateTag(Block.Stunned)` → switching пропускает голову.
- **Magic shield на торсе игрока:** магическая атака врага использует `BlockedStateTags=[Block.Shield.Magic]` → точка торса исключается.
- **Раненый персонаж не достаёт до головы дракона:** `HeadPoint.BlockedSourceTags = {Source.Wounded}` → target system не лочит голову если source-actor имеет тег `Source.Wounded`.

### R3. Единый интерфейс targetable

```cpp
class ITargetSystemInterface
{
    virtual bool IsTargetable() const;
    virtual TArray<UTargetPointComponent*> GetTargetPoints() const;
    virtual TArray<UTargetPointComponent*> QueryTargetPoints(const FTargetPointQuery& Query) const;
    virtual void OnTargetLockBegin(AActor* LockOwner);
    virtual void OnTargetLockEnd(AActor* LockOwner);
};
```

- `ITargetSystemTargetableInterface` и `ITargetSystemOwnerInterface` — удалены, всё слито сюда.
- `FTargetPointQuery` — структура с `PointTagQuery`, `StateTagQuery`, `SourceTags` (как `FMotionWarpingPointQuery`).
- `OnTargetLockBegin/End` — callback'и для проектного UI / VFX / AI-reaction. Сейчас этого вообще нет.

### R4. GameplayTargetingSystem — единственный pipeline

В `UTargetSystemComponent`:
- Удаляется `FindNearestTarget`, `SortPotentialTargetsByDistance`, `SortPotentialTargetsByAngle`, `AddPotentialTargetsByInterface`, `LineTrace`, `IsInViewport`, `ObjectIsTargetable`, `GetAngleUsingCameraRotation`, `GetAngleUsingCharacterRotation`.
- Всё это покрывается `UTargetingPreset` с набором task'ов.
- `TargetingPreset : TObjectPtr<UTargetingPreset>` — required UPROPERTY (не opt-in).
- Component держит только: lock state, control rotation, widget, **результат** последнего targeting query.

### R5. Composable Targeting Tasks

Расширить набор `UTargetingTask_*` (все в `Source/TargetSystem/Public/Targeting/`):

| Task | Type | Назначение |
|---|---|---|
| `UTargetingTask_CollectTargetables` | Selection | Поиск actor'ов, реализующих `ITargetSystemInterface::IsTargetable() == true`, в радиусе SourceActor |
| `UTargetingTask_FilterByLineOfSight` | Filter | Trace до actor'а, отсекает за стенами |
| `UTargetingTask_FilterByViewport` | Filter | Проверка `WasRecentlyRendered` / projection в viewport |
| `UTargetingTask_FilterByMaxDistance` | Filter | Distance cap |
| `UTargetingTask_FilterByMaxAngle` | Filter | Угловой cone от camera/control rotation |
| `UTargetingTask_SortByLockOn` | Sort | Текущий `TST_TargetLock`, переименован. Score = ScreenOffset × ScreenWeight + Distance × DistanceWeight |
| `UTargetingTask_FilterSwitchTargetSide` | Filter | Текущий `TFT_SwitchTargetLock`, переименован. Отсекает actors не на той стороне |
| `UTargetingTask_SelectTargetPoint` | Selection | Hit→Point, как `UTargetingTask_SelectMotionWarpingPoint` в MW (использует `QueryTargetPoints` интерфейса) |
| `UTargetingTask_SortByScreenX` | Sort | Сортировка по экранной X (для switching между точками на одной цели) |

Контекст через `FTargetingSourceContext` + `UTargetLockContext : UObject` (как сейчас) — расширить полями:
- `ETargetSwitchMode Mode` (`LockOn` / `SwitchLeft` / `SwitchRight` / `SwitchPoint`)
- `AActor* CurrentTarget`
- `UTargetPointComponent* CurrentPoint`

### R6. Replication

`UTargetPointComponent::StateTags` — replicated push-model. Уже определено в `NextMotionWarping` Requirements R4 — копируем pattern 1:1.

`TargetingPreset` — design-time, не реплицируется.

`Locked target actor` — реплицируется по необходимости (если game использует ASC-based lock-on visualization). Сейчас singleplayer Godreaper — не приоритет, но закладываем поле как реплицируемое.

### R7. Дебаг

**Отдельный модуль `TargetSystemDebug`** (Runtime, Default), по образцу `NextBlockActionsDebug`:

- GameplayDebuggerCategory `TargetSystem` (хоткей в PIE).
- Показывает на selected pawn:
  - Текущий locked target + рисует линию + screen-projection box на нём
  - Все candidate'ы (текущий результат targeting request'а) с их `Score`
  - Все точки выбранного target'а с `PointTags` + `StateTags` (цвет: зелёная = доступна по последнему query, красная = отфильтрована)
- Зависит от `GameplayDebugger` плагина.

**Console var:** `targetsystem.DebugDraw` (int 0/1/2/3) для quick toggle без открытия Debugger UI.

### R8. Лёгкая интеграция (setup story)

Setup в новый проект — **4 шага, не больше:**

1. Подключить плагин (uplugin).
2. На pawn'е игрока добавить `UTargetSystemComponent` + назначить `TargetingPreset` asset (пример preset'а описан в README).
3. На enemy actor'ах: implement `ITargetSystemInterface`, вернуть `IsTargetable() = true`.
4. Bind input → `Component->TryStartTargetLock()` / `SwitchTarget(Axis)`.

Точки — опциональны (`GetTargetPoints()` может вернуть `{}` — будет использоваться `GetActorLocation()` как fallback).

**Не требуется:** demo content, mannequin assets, example map, default config с input mappings. Всё это — на стороне проекта.

---

## Архитектура

```mermaid
classDiagram
    class UTargetSystemComponent {
        <<Runtime component on Pawn>>
        +TargetingPreset : UTargetingPreset
        +TryStartTargetLock()
        +SwitchTarget(Axis)
        +SwitchTargetPoint(Axis)
        +StopTargetLock()
        +OnTargetLockedOn / Off : delegates
        -LockedActor / LockedPoint : state
    }

    class UTargetPointComponent {
        <<SceneComponent on target>>
        +PointTags : FGameplayTagContainer
        +StateTags : FGameplayTagContainer (replicated)
        +MetadataTags : FGameplayTagContainer
        +LockOnPitchOffsetCurve : UCurveFloat
        +AddStateTag / RemoveStateTag
        +MatchesQuery(...)
    }

    class ITargetSystemInterface {
        <<interface on enemy>>
        +IsTargetable()
        +GetTargetPoints()
        +QueryTargetPoints(Query)
        +OnTargetLockBegin / End
    }

    class UTargetingPreset {
        <<UE GameplayTargetingSystem>>
        -SelectionTasks []
        -FilterTasks []
        -SortTasks []
    }

    class UTargetLockContext {
        <<UObject, lifecycle per request>>
        +Mode : ETargetSwitchMode
        +CurrentTarget
        +CurrentPoint
    }

    class FTargetingDebuggerCategory_TargetSystem {
        <<TargetSystemDebug module>>
        +draw locked target + candidates + points
    }

    UTargetSystemComponent ..> UTargetingPreset : uses
    UTargetSystemComponent ..> UTargetLockContext : creates per request
    UTargetSystemComponent ..> ITargetSystemInterface : queries
    ITargetSystemInterface ..> UTargetPointComponent : returns
    UTargetingPreset ..> "UTargetingTask_*" : composes
    FTargetingDebuggerCategory_TargetSystem ..> UTargetSystemComponent : reads state
```

### Тег-конвенция (native в плагине)

**Правило (hard rule):** ВСЕ native gameplay-теги, объявляемые плагином, ОБЯЗАНЫ начинаться с префикса `TargetSystem.`. Без исключений. Это касается и текущей работы, и любых будущих расширений.

Префиксы первого уровня, которые плагин владеет:

- `TargetSystem.Point.*` — категории точек (semantic identification).
- `TargetSystem.Block.*` — runtime-блокеры точек (state-теги, реплицируются).
- `TargetSystem.Meta.*` — design-time metadata (`Meta.PreferredForLockOn`, `Meta.NoAutoSwitch`).
- `TargetSystem.Source.*` — теги, которые плагин ожидает увидеть на source-actor'е для source-side фильтра (опционально, project-side маппинг).

**Только префиксы.** Конкретные subtag'и (`Point.Head`, `Point.Tail`, `Block.Stunned`, `Source.Wounded`) объявляются project-side в `Config/Tags/*.ini` или `DefaultGameplayTags.ini`. Плагин их не знает.

**Где это enforce-ится:**

- UPROPERTY на компоненте/struct'е использует `meta=(Categories="TargetSystem.Point")` (и аналоги) — селектор тега в редакторе показывает только дочерние теги.
- Code review — любое объявление native-тега без `TargetSystem.` префикса блокирует merge.
- CI lint (будущее) — grep по `UE_DEFINE_GAMEPLAY_TAG*` в плагине, проверка префикса.

---

## Use cases

### UC1 — Базовый lock-on на ближайшего врага

1. Player жмёт `LockOn` input.
2. `Component->TryStartTargetLock()` → async targeting request с `Preset_LockOn` (содержит `CollectTargetables` + `FilterByLineOfSight` + `FilterByMaxDistance` + `SortByLockOn`).
3. Топ-1 actor становится locked target. `OnTargetLockedOn` fires.
4. Camera control rotation крутится к target.
5. Player снова жмёт `LockOn` (или умирает target) → `StopTargetLock`.

### UC2 — Switch target по стику (другой враг)

1. Player тянет правый стик влево.
2. `Component->SwitchTarget(AxisX < 0)` → sync targeting request с `Preset_SwitchTarget` (содержит `FilterSwitchTargetSide` с `Mode=SwitchLeft` в context).
3. Берётся первый actor из результата → новый locked target.

### UC3 — Switch точки на текущем target'е (босс-дракон)

1. Player тянет левый стик влево.
2. `Component->SwitchTargetPoint(AxisX < 0)` → sync targeting request с `Preset_SwitchPoint` (`SelectTargetPoint` от текущего target'а + `SortByScreenX`).
3. Текущая точка → следующая слева в screen-sorted списке. Lock-widget переезжает.

### UC4 — Точка временно недоступна (stunned-голова дракона)

1. Ability stun накладывает на дракона state-тег → ability вызывает `HeadPoint->AddStateTag(TargetSystem.Block.Stunned)`.
2. Свич точек теперь пропускает голову (preset фильтрует по `BlockedStateTags=[Block.Stunned]`).
3. Через N секунд `RemoveStateTag` — голова снова в выборке.

### UC5 — Lock-on игнорирует «полупрозрачных» врагов (фаза призрака)

1. Враг в фазе призрака возвращает `IsTargetable() = false`.
2. `CollectTargetables` task его не подбирает.
3. Если уже был залочен — отдельный watchdog в `UTargetSystemComponent` тикает и снимает lock когда `IsTargetable` стал false.

### UC6 — Source-side фильтр: раненый игрок не лочит голову дракона

1. Игрок имеет gameplay-тег `Source.Wounded` (от какой-то ability/state).
2. На точке `HeadPoint` дракона design-time выставлено `BlockedSourceTags = "Source.Wounded"`.
3. При запросе на switch точки игрок передаёт свой actor tags в `FTargetPointQuery.SourceTags`.
4. `MatchesQuery` исключает голову — switching пропускает её. По R2 `BlockedSourceTags` — на стороне точки.
5. После исцеления (тег ушёл) — голова снова доступна.

---

## План работы

Каждый шаг — отдельный коммит. Acceptance criterion на каждый.

| # | Шаг | Verify |
|---|---|---|
| 1 | **Cleanup: drop Content, исправить uplugin.** Удалить `Content/`, `Config/DefaultInput.ini`. Поправить `.uplugin` (VersionName 2.0.0, drop PlatformAllowList, `CanContainContent: false`, добавить debug module entry). | Плагин собирается, ничего не сломано в проектах-потребителях (Bogatyr использует только classes, не Content). |
| 2 | **L9 fix.** Переместить `TFT_SwitchTargetLock.cpp` → Public/, `.h` → Private/ (вообще пересмотреть — selection/filter tasks обычно полностью в Public). | Plugin собирается. |
| 3 | **R1 + R2 — `UTargetPointComponent` + Query.** Новый класс с `PointTags`/`StateTags`/`MetadataTags`/`RequiredSourceTags`/`BlockedSourceTags`/`LockOnPitchOffsetCurve`. Структура `FTargetPointQuery`. Метод `MatchesQuery`. CoreRedirect `UBTargetPoint → UTargetPointComponent`. | Существующие BP с `BTargetPoint` автоматически конвертируются через CoreRedirect. На тестовой сцене точки видны, теги настраиваются, `MatchesQuery` правильно фильтрует на тестовых данных. |
| 4 | **R6 — replication `StateTags`.** push-model. | Standalone работает, single-player пайплайн не сломан. |
| 5 | **R3 — единый `ITargetSystemInterface`.** Слить `TargetableInterface` и `OwnerInterface` сюда. Добавить `QueryTargetPoints(FTargetPointQuery)`, `OnTargetLockBegin/End`. Удалить старые интерфейсы. | Bogatyr (внешний потребитель): проверить что `BBaseCharacter::IsTargetable` / lock-on на врагов работает. CoreRedirect для функций где возможно. |
| 6 | **L6 — drop `UTargetSystemDependencies` + `FTargetActorDetails`.** Точки запрашиваются через интерфейс. | Что было `GetTargetSystemDependencies()->GetTargetActorDetails()->TargetPoints` — теперь `Interface->GetTargetPoints()`. |
| 7 | **R5 — Targeting Tasks набор.** Реализация всех 9 task'ов из таблицы. Переименование `TST_*` → `UTargetingTask_SortByLockOn`, `TFT_*` → `UTargetingTask_FilterSwitchTargetSide`. `UTargetingTask_SelectTargetPoint` использует `FTargetPointQuery` из R2. CoreRedirects. | Тестовый Preset собирается из новых task'ов, lock-on работает на тестовой сцене. |
| 8 | **R4 — refactor `UTargetSystemComponent`.** Удалить весь ручной target search. Слить `UNextTargetSystemComponent` обратно в base. `TargetingPreset` — required. Только camera/widget/lock state. | Lock-on на Bogatyr работает только через TargetingPreset, fallback `bUseTargetSubsystem` удалён. |
| 9 | **Native gameplay tags.** `TargetSystem.Point.*`, `TargetSystem.Block.*`, `TargetSystem.Meta.*` префиксы (только префикс, без конкретики). | Categories meta фильтрует тег-селектор в редакторе. |
| 10 | **R7 — debug module.** Новый `TargetSystemDebug` module + GameplayDebuggerCategory. Console var `targetsystem.DebugDraw`. | В PIE категория показывает locked target + candidates + scores + точки с тегами. |
| 11 | **R8 — обновить README.** Setup в 4 шага, краткий пример preset'а, drop upstream-wiki ссылок где они больше не релевантны (потому что pipeline другой). Сохранить структуру (mklabs origin + acknowledgement). | README ≤ текущий размер, новый user может настроить плагин за 5 минут чтения. |
| 12 | **Cleanup CoreRedirects.** Консолидировать `DefaultTargetSystem.ini`, оставить только редиректы из v1.x → v2.0. Удалить дубликаты. | `.ini` чистая. |
| 13 | **CHANGELOG v2.0.** Заполнить `CHANGELOG.md` с breaking changes списком. | Документ есть. |

---

## Не входит в scope

- **Combat / damage / abilities** — GAS / `NGCombatSystem`, не зона targeting'а.
- **AI восприятие / враждебность** — игра, не плагин.
- **UI lock widget** — design-time UMG asset на стороне проекта. Плагин даёт примеру в README (template-class).
- **OverrideCameraDistanceVolume** — открытый вопрос Q1, скорее всего **выносится** в проект.
- **Multi-lock** (несколько одновременно залоченных целей) — не v2.0.
- **Network replication lock state** — закладываем структуру под, но активно не имплементим для v2.0 (Godreaper singleplayer).
- **Editor Component Visualizer** для точек — stretch, после release v2.0.

---

## Открытые вопросы

| # | Вопрос | Варианты |
|---|---|---|
| Q1 | **OverrideCameraDistanceVolume** — оставлять в плагине? | (a) выкинуть в проект Bogatyr — это про геймплейные триггер-зоны, не targeting; (b) оставить как opt-in side-feature; (c) вынести в отдельный sub-plugin `TargetSystemCamera` |
| Q2 | **Lock widget** — оставлять `WidgetComponent` логику в плагине или вынести в проект? | (a) оставить spawn/attach логику, asset class берётся из `LockedOnWidgetClass` UPROPERTY (как сейчас); (b) убрать целиком, проект делает spawn сам через `OnTargetLockedOn` |
| Q3 | **`UMG` / `Slate` / `SlateCore` dependencies** — нужны ли? | Зависит от Q2. Если widget остаётся — нужен `UMG`. `Slate`/`SlateCore` — на проверку, скорее всего можно убрать |
| Q4 | **Naming компонента** — `UTargetSystemComponent` или новое имя? | (a) оставить `UTargetSystemComponent` (CoreRedirect не нужен); (b) переименовать в `UTargetLockComponent` (точнее отражает суть — это камера-локон, не «вся система таргетинга») |
| Q5 | **`PitchOffsetCurve` на точке** — нужно ли в v2.0? | Сейчас используется в Godreaper? Если нет — выкинуть. Если да — оставить, но переименовать в `LockOnPitchOffsetCurve` |
| Q6 | **Stable preset для switching между точками одной цели — отдельный preset или task в base preset'е с `Mode`?** | (a) отдельный `UTargetingPreset` slot на компоненте (`Preset_LockOn`, `Preset_SwitchTarget`, `Preset_SwitchPoint`); (b) один preset, behaviour task'ов меняется по `UTargetLockContext::Mode` |
| Q7 | **Replication strategy для `LockedActor`** — заложить структуру сейчас или пропустить? | (a) добавить replicated field сразу (минимум кода); (b) отложить до сетки |
| Q8 | **Backward-compat policy.** | Сколько CoreRedirects держим? Все из v1 → v2, или только критичные? Сколько version'ов поддерживаем после v2.0 — нужен migration guide или break clean? |

---

## References

- [Plugins/TargetSystem/](../) — текущий код.
- [Plugins/TargetSystem/README.md](../README.md) — текущая документация v1.
- [Plugins/NextMotionWarping/Docs/Requirements.md](../../NextMotionWarping/Docs/Requirements.md) — аналогичный requirements-doc для MW (одна и та же модель `PointTags`/`StateTags`/`Query`).
- [Plugins/NextBlockActions/README.md](../../NextBlockActions/README.md) — стиль readme/debug module, на который ориентируемся.
- **Upstream:** [mklabs/ue4-targetsystemplugin](https://github.com/mklabs/ue4-targetsystemplugin) — origin.
- **UE Docs:** `UTargetingPreset`, `UTargetingSelectionTask_*`, `UTargetingFilterTask_*`, `USimpleTargetingSortTask`, `FTargetingSourceContext`, `FTargetingRequestHandle`.
