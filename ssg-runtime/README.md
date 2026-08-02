The `ssg-runtime` static library contains cross-platform backend and utility
code shared by the game and developer tools.

| Directory | Purpose |
| --- | --- |
| `sys/` | System wrappers – buffer, file, path, thread, log, input |
| `gfx/` | Graphics layer – coordinates, surfaces, text, BMP, GPU/window backends |
| `audio/` | Audio layer – sound, BGM/MIDI, codecs, volume, audio backends |
| `util/` | General utilities – cast, endian, enum helpers, hash, guard, math, time, debug |

