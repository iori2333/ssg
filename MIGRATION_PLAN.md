# Architecture migration status

## Composition root

`GameApplication` is the application-lifetime composition root:

```text
GameApplication
  |-- context: GameContext
  |   |-- data, graphics, sound, and music services
  |   |-- record persistence and playback
  |   |-- stage loader and active StageSession
  |   |-- player, enemies, bullets, items, and effects
  |   `-- application-wide UIManager
  `-- flow: gameflow::GameFlow
      `-- exactly one active variant state
```

`GameApplication` is private to `app/entry.cpp`; gameplay and UI modules do
not access it as a service locator. `GameContext` contains systems only. UI
scenes are constructed and destroyed with their active flow state.

## Flow state model

`gameflow::GameFlow` is the sole screen-transition authority. Its active state
is a closed `std::variant`, and states return semantic `FlowEvent` values
instead of installing frame callbacks or mutating a parallel state enum.

```text
Project -> Title
Title -> WeaponSelect -> Gameplay(Live)
Title -> ReplayBrowser -> Gameplay(Replay)
Title -> Score / MusicRoom / BulletGallery
Title -> Gameplay(Demo)

Gameplay
  |-- mode: Live / Replay / Demo
  `-- phase: Running / Paused / GameOverIntro / GameOverMenu
```

Live, Replay, and Demo share the same gameplay step and rendering order.
Pause and Game Over are phases of the active gameplay state rather than
independent application screens.

## Completed migrations

- Removed the global `GameFlowManager`, mutable frame-function routing, and
  the duplicate `GameState` tag.
- Removed `game_main.*`; front-end flow and gameplay flow now have separate,
  cohesive implementations.
- Moved application ownership from `GameFlow.ctx` into `GameApplication`.
- Changed Player, RecordSystem, menus, and UI scenes to use explicit
  dependencies, inputs, actions, and completion results.
- Kept automatic deathbomb input in the effective frame input so Replay
  recording sees the same bomb input as gameplay.
- Preserved the gameplay update order and the Stage-transition early-return
  boundary across Live, Replay, and Demo modes.
- Replaced scene completion callback chains with explicit score and Replay
  result-flow states.
- Kept `GameData` as the sole validated owner of PAK data and retained the
  context-owned Stage, Enemy, Bullet, Item, Effect, Player, and Record systems.

## Remaining process-wide state

Low-level input, audio, graphics-surface, and geometry backends remain global
platform abstractions. `AppConfig()` also remains the persistent configuration
owner. These are no longer used to route application screens.

## Stage validation

`stage_validator` validates embedded programs, timeline boundaries, and maps
extracted from `MAP.PAK`:

```powershell
build\bin\stage_validator.exe build\map_inspect
```

The title-screen random Demo is not a Stage/Scroll acceptance test because its
recorded input may differ after stage configuration changes.
