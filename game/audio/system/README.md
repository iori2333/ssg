# Audio system redesign

This directory contains the application-owned audio system introduced on the
`audio-system-redesign` branch. The new architecture is compiled into
`game_runtime`, but the existing `GIAN07` call sites still use the legacy
global audio API until the UI and gameplay entry points are migrated.

## Layout

- `core/` - audio errors, playback states, snapshots, and lock-free volume ramp.
- `midi/` - pure MIDI parser, stateful sequencer, and FluidSynth backend.
- `stream/` - waveform BGM data source and miniaudio-backed player.
- `sfx/` - miniaudio-backed sound effect bank.
- `backend/` - miniaudio engine wrapper.
- `bgm/` - BGM controller that owns waveform/MIDI mode selection and fades.
- `system/audio_system.h` - composition root exposing the public audio API.

## Migration status

- New components build and link successfully.
- The old `game/audio` modules remain the active runtime path.
- Next step is migrating `GIAN07/audio`, `GIAN07/music`, and the audio UI
  call sites from the legacy global functions to `audio::AudioSystem`.

