# AGENTS.md

## Build system

- **Source of truth:** CMake + Ninja (`CMakeLists.txt`, `build_windows.bat`, `build_linux.sh`).
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
- Requires `pkg-config`, `pangocairo`, and `fontconfig`.

## Submodules

All vendored dependencies live under `libs/` as Git submodules:

- `libs/SDL3`
- `libs/miniaudio`
- `libs/BLAKE3`
- `libs/dr_libs`
- `libs/libogg`
- `libs/libvorbis`
- `libs/libwebp`
- `libs/tomlplusplus`
- `libs/tinysoundfont`

Build scripts run `git submodule update --init --recursive`. Do not hand-edit vendored submodule code.

## Architecture

| Directory | Purpose |
| --- | --- |
| `GIAN07/` | Game logic layer – derived from pbg's original source, refactored to C++23 with modern STL usage |
| `game/` | Cross-platform layer: SDL3 graphics, miniaudio/TSF audio backends, sys/util helpers, Windows resources, and the `main.cpp` entry point |
| `game/sys/` | System wrappers – buffer, file, path, thread, log, input |
| `game/gfx/` | Graphics layer – coordinates, surfaces, text, BMP, SDL3 window/render backends |
| `game/audio/` | Audio layer – sound effects, MIDI, BGM, codecs, volume, audio backends |
| `game/util/` | General utilities – cast, endian, enum helpers, hash, guard, math, time, debug |
| `game/platform/` | Platform-specific code with no cross-platform equivalent (text rendering only) |
| `game/platform/windows/` | Win32 GDI text rendering |
| `game/platform/linux/pangocairo/` | PangoCairo text rendering on Linux |
| `cmake/` | CMake helper modules – `bin2h.cmake` (binary→C array) and `generate_scripts_data.cmake` (ECL/SCL embed pipeline) |
| `scripts/` | ECL/SCL script source files – assembled by `script_tool` at build time and embedded as C arrays via `scripts_data.cpp` |
| `tools/` | Build tools: `pack_tool` (DAT/PAK pack manipulation + music data migration), `script_tool` (ECL/SCL disasm/asm) |
| `bin/` | Runtime data directory: `IMAGES.PAK`, `MAP.PAK`, `MUSIC.PAK`, `SOUND.PAK`, `SSG.TOML`, soundfonts, replays (gitignored) |

Entry point: `game/main.cpp`.

### Music data format

Original BGM stored in `MUSIC.PAK` (PBG format, unified entries):
```
[title_len:u32LE][title:UTF-8][comment_len:u32LE][comment:UTF-8\n][midi:raw SMF]
```

Legacy comments previously in `ENEMY.DAT` entries 027–046 have been migrated
to `MUSIC.PAK`. The remaining map/demo data has been repacked as `MAP.PAK`.
`GRAPH.DAT` and `GRAPH2.DAT` have been merged into `IMAGES.PAK`.
Convert original GBK-encoded data with `pack_tool extract-music` → `pack_tool pack-music`.

## Tooling

- **C++ standard:** C++23, extensions off.
- **Formatter:** `.clang-format` uses `BasedOnStyle: LLVM`.
- **Linter:** `.clang-tidy` is configured; see the file for enabled/disabled checks.
- **No tests or CI** are currently wired up.

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

## Editing workflow

After editing any `.cpp` / `.h` file under `GIAN07/`, `game/`, or `tools/`,
run the following before considering the change done:

1. **Format** — apply LLVM style with `.clang-format`:
   ```sh
   clang-format -i path/to/edited.cpp path/to/edited.h
   ```
   Re-stage the file afterwards; `clang-format` may reorder or reflow lines.

2. **Lint** — run `.clang-tidy` on the edited files.  Use the compile
   commands exported by CMake:
   ```sh
   cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release  # once
   clang-tidy -p build path/to/edited.cpp
   ```
   Fix any new warnings introduced by your change.  Pre-existing warnings
   in legacy parts of `GIAN07/` are tolerated, but do not add more.

3. **Build** — make sure the project still compiles end-to-end:
   ```sh
   ./build_linux.sh          # or build_windows.bat
   ```
   Catch missing includes, signature mismatches, and toolchain breakage
   that the linter may miss.

Skip these only for non-code edits (documentation, CMake, scripts, the
`scripts/` source tree, vendored submodule code).

