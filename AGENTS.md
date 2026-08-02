# AGENTS.md

## Build system

- **Source of truth:** CMake + Ninja (`CMakeLists.txt`, subdirectory
  `CMakeLists.txt` files, `build_windows.bat`, `build_linux.sh`).
- The root `README.md` still says Tup and `bin/` output — that is stale.
- Generated output is `build/bin/GIAN07` (Linux) or `build/bin/GIAN07.exe` (Windows).

### Windows

```bat
# Run from a Visual Studio "x64_x86 Cross Tools Command Prompt"
build_windows.bat
```

- Requires Visual Studio 2022+, CMake ≥3.21, Ninja, Git.
- Script auto-detects VS and runs `vcvarsall.bat x64_x86`.
- Static CRT is forced to match static SDL3.

### Linux

```sh
./build_linux.sh
```

- Supports GCC ≥15 and Clang ≥18.
- Build script honors `CC`/`CXX` if set; otherwise falls back to system `cc`.
- FreeType and HarfBuzz are built from the vendored SDL_ttf submodule.
- Requires a system-installed Noto Sans CJK font collection. `SSG_CJK_FONT`
  overrides its path when it is outside the standard distribution locations.

## Submodules

All vendored dependencies live under `libs/` as Git submodules:

- `libs/SDL3`
- `libs/miniaudio`
- `libs/libwebp`

Build scripts run `git submodule update --init --recursive`. Do not hand-edit vendored submodule code.

## Architecture

| Directory | Purpose |
| --- | --- |
| `ssg/` | Game executable source (late-90s/early-2000s pbg code and modern application composition) |
| `ssg/app/` | Application entry point, composition root, subsystem initialization, and live display settings |
| `ssg/data/` | Validated PBG/PAK ownership, music catalog data, and graphics/SFX loading adapters |
| `ssg/gameflow/` | Variant-based screen routing and live/replay/demo gameplay flow |
| `ssg/gameplay/` | Shared gameplay rules, session state, rank policy, and playfield geometry |
| `ssg/item/` | Collectible item entities, movement, pickup rules, and rendering |
| `ssg/music/` | Music playback, track metadata, and replacement BGM pack selection |
| `ssg/record/` | Score and Replay persistence, input recording, and Replay playback |
| `ssg/settings/` | Persistent application configuration models and TOML serialization |
| `ssg/stage/` | Validated SCL/MAP parsing, stage asset installation, timeline execution, and background scrolling |
| `ssg/ui/` | Application-wide UI ownership: scenes, menus, HUD, and message windows |
| `ssg-runtime/` | Cross-platform static runtime: SDL3/miniaudio backends and I/O utilities |
| `ssg-runtime/sys/` | System wrappers – bit streams, buffer, file, path, thread, log, input |
| `ssg-runtime/gfx/` | Graphics layer – coordinates, surfaces, text, BMP, GPU/window backends |
| `ssg-runtime/audio/` | Audio layer – sound effects, BGM/MIDI, codecs, volume, audio backends |
| `ssg-runtime/util/` | General utilities – cast, endian, enum helpers, hash, guard, math, time, debug |
| `ssg-scripts/` | ECL/SCL and i18n sources, conversion tool, generators, and generated-data static library |
| `tools/` | Developer tools: pack_tool and stage_validator |

Entry point: `ssg/app/main.cpp`.

### Game data format

Runtime resources can be loaded from either an extracted `data/` directory or
the PBG-compressed `data.pak`. If `data/` exists next to the executable, it is
always used; `data.pak` is only opened when the directory is absent. An invalid
directory is an error and does not silently fall back to the archive.

The directory uses `maps/*.map`, `images/*.bmp`, `music/*.mid`,
`sounds/*.wav`, and `demos/*.dat`. Entries in each section are numbered
contiguously from `000`. Music titles and comments live only in the embedded
i18n catalogs.

Archive entry 0 is a versioned manifest; the remaining entries contain the
same five sections in manifest order.

Manifest v2 uses little-endian integers:

```
magic[8] = "SSGDATA\x1a"
version:u32 = 2
section_count:u32 = 5
repeat section_count:
  section_id:u32
  first_entry:u32
  entry_count:u32
```

Use `pack_tool extract data.pak <directory>` to produce the five section
directories and `pack_tool pack <directory> data.pak` to rebuild the archive.
Debug Demo Recording writes local Replay v2 files to `demos/`; it never edits
`data.pak`. Local demos take precedence during Debug Demo Play and are copied
into an extracted `demos/` section only when explicitly repacking.

## Tooling

- **C++ standard:** C++23, extensions off.
- **Formatter:** `.clang-format` uses `BasedOnStyle: LLVM`.
- **Linter:** `.clang-tidy` is configured; see the file for enabled/disabled checks.
- **No tests or CI** are currently wired up.

### Stage validation

`stage_validator` validates the embedded SCL programs and can additionally
validate real maps extracted from the `maps` section of `data.pak`:

```powershell
build\bin\pack_tool.exe extract bin\data.pak build\data_inspect
build\bin\stage_validator.exe build\data_inspect\maps
```

Do not use the title-screen random Demo as the primary Stage/Scroll regression
test. Its replay input can diverge after stage configuration changes.

## Include guidelines

All `#include` directives in `.cpp` / `.h` files must be grouped into
**four blocks**, separated by a blank line between blocks.  No blank lines
inside a block.  Within each block, includes are sorted alphabetically.

| Block | Contents |
| --- | --- |
| 1 | System / standard library headers (`<cstdint>`, `<vector>`, `<format>`, …) |
| 2 | Third‑party library headers (`<SDL3/…>`, `<toml++/toml.hpp>`, `<miniaudio.h>`, `<windows.h>`, …) |
| 3 | Project headers from the **same directory or a subdirectory** of the source file — quoted, relative path (e.g. `"graphics.h"` instead of `"gfx/graphics.h"`) |
| 4 | Other project headers (from any other include‑root directory) — quoted, full path from the root (e.g. `"gfx/graphics.h"`, `"sys/file.h"`) |

### C‑style → C++ headers

In C++ files, replace C standard‑library headers with their C++ counterparts
(e.g. `<assert.h>` → `<cassert>`, `<stddef.h>` → `<cstddef>`).

### Conditional includes

An `#include` guarded by `#if` / `#ifdef` / `#elif` / `#ifndef` must keep its
guard.  Treat the whole guarded block as a single include unit — it stays
where the guard logic requires it.

## Known gotchas

- `.vscode/launch.json` is stale: it references tasks that no longer exist in `.vscode/tasks.json` and expects binaries in `bin/` instead of `build/bin/`.
- `README.md` mentions `install_linux.sh` and a Tup-based workflow; neither exists in this branch.

