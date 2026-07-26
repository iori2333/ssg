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
      |-- stages: StageLoader
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
- Moved stage runtime installation to `stage::StageLoader`.
- Moved track selection and playback orchestration into the context-owned
  `TrackManager`; music metadata remains data owned by `GameData`.
- Removed the legacy menu/window controller family and consolidated
  application UI under `UIManager`.
- Moved bullet, item, player, game, ending, score, demo, and UI ownership into
  `GameContext`.

## Remaining global runtime systems

The remaining high-level global instances are:

- `GameFlow`: application state-transition coordinator and composition root.
- `Enemies`: enemy runtime and ECL/SCL execution state.
- `Bosses`: boss runtime state.
- `Scroller`: scene and map scrolling runtime state.
- `Effects`: gameplay visual-effect runtime state.

These systems already accept several dependencies through `Bind()` or explicit
method parameters. Future migration should move ownership one subsystem at a
time without introducing compatibility globals.

Low-level process-wide backends such as input state, audio playback, graphics
surfaces, and geometry rendering remain global platform abstractions for now.
