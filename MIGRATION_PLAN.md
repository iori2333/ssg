# Architecture migration status

## Composition root

Application-owned systems are composed under `GameFlow.ctx`:

```text
GameFlow (global transition coordinator)
  `-- ctx: GameContext
      |-- data: GameData
      |-- graphics: GraphicsLoader
      |-- sound_effects: SfxLoader
      |-- tracks: TrackManager
      |-- stage_loader: StageLoader
      |-- stage: StageSession
      |-- bullets: BulletManager
      |-- items: ItemManager
      |-- game: GameManager
      |-- player: Player
      |-- ending: EndingManager
      |-- scores: ScoreManager
      |-- demos: DemoManager
      `-- ui: UIManager
```

`GameData` is the sole owner of `MAP.PAK`, `IMAGES.PAK`, `MUSIC.PAK`, and
`SOUND.PAK`. It validates archives and exposes data-only extraction and music
catalog APIs. Graphics, sound, track, demo, and stage services receive that
owner explicitly; there are no data-layer global manager instances.

`UIManager` owns menu controllers, menu trees, replay-list storage, and the
message window. Callers use its semantic APIs rather than concrete global UI
objects.

## Completed migrations

- Removed the legacy loader and the later global `PackManager`, `GfxManager`,
  `MusicManager`, `SfxManager`, and `StageManager` replacements.
- Separated required archive validation from optional audio-device startup.
- Moved stage asset selection and runtime installation to
  `stage::StageLoader`.
- Replaced the legacy `Scroller` and raw SCL cursor with `StageSession`, which
  owns the validated scene timeline and background runtime. `SceneRunner`
  owns stage-frame advancement; `StageBackground` owns map scrolling and
  special background modes.
- Ending owns a separate `SceneRunner`; it no longer borrows mutable stage
  state or clears the active stage background when the ending begins.
- Moved track selection and playback orchestration into the context-owned
  `TrackManager`; music metadata remains data owned by `GameData`.
- Removed the legacy menu/window controller family and consolidated
  application UI under `UIManager`.
- Moved bullet, item, player, game, ending, score, demo, and UI ownership into
  `GameContext`.

## Remaining global runtime systems

The remaining high-level global instances are:

- `GameFlow`: application state-transition coordinator and composition root.
- `Enemies`: enemy runtime and ECL execution state.
- `Bosses`: boss runtime state.
- `Effects`: gameplay visual-effect runtime state.

These systems already accept several dependencies through `Bind()` or explicit
method parameters. Future migration should move ownership one subsystem at a
time without introducing compatibility globals.

Low-level process-wide backends such as input state, audio playback, graphics
surfaces, and geometry rendering remain global platform abstractions for now.

## Stage validation

`stage_validator` validates all embedded SCL programs, timeline boundary
behavior, and optionally real maps extracted from `MAP.PAK`:

```powershell
build\bin\stage_validator.exe build\map_inspect
```

The title-screen random Demo is not a Stage/Scroll acceptance test. Its replay
data may no longer match the current stage configuration, so deterministic
stage entry and the validator are the supported verification paths.
