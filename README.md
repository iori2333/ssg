# 秋霜玉

Source code for **Shuusou Gyoku**, the first Touhou Project-adjacent game by Project Shrine Maiden (pbg).

## Building

This project uses **CMake + Ninja**. All vendored dependencies are Git submodules under `libs/`.

Binaries are written to `build/bin/`:

- Windows: `build/bin/GIAN07.exe`
- Linux: `build/bin/GIAN07`

### Windows

Requirements: Visual Studio 2022+, CMake ≥3.21, Ninja, Git.

Run from a Visual Studio **x64_x86 Cross Tools Command Prompt**:

```bat
build_windows.bat
```

The script initializes submodules, configures CMake, and builds a Release binary.

### Linux

Requirements: GCC ≥15 or Clang ≥18, CMake ≥3.21, Ninja, Git, `pkg-config`, `pangocairo`, `fontconfig`.

```sh
./build_linux.sh
```

The build script honors `CC`/`CXX` if set; otherwise it falls back to the system `cc`.

### Configurations

Both scripts build Release by default. To build Debug, run CMake manually:

```sh
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

## Project layout

| Directory | Purpose |
| --- | --- |
| `GIAN07/` | Game logic layer – derived from pbg's original source, refactored to C++23 with modern STL usage |
| `game/` | Cross-platform layer: SDL3 graphics, miniaudio + TinySoundFont audio, sys/util helpers, Windows resources, and the `main.cpp` entry point |
| `game/sys/` | System wrappers – buffer, file, path, thread, log, input |
| `game/gfx/` | Graphics layer – coordinates, surfaces, text, BMP, SDL3 window/render backends |
| `game/audio/` | Audio layer – sound, MIDI, BGM, codecs, volume, miniaudio/TSF backends |
| `game/util/` | General utilities – cast, endian, enum helpers, hash, guard, math, time, debug |
| `game/art/` | Windows resource icons |
| `game/platform/` | Platform-specific code with no cross-platform equivalent (text rendering only) |
| `game/platform/windows/` | Win32 GDI text rendering |
| `game/platform/linux/pangocairo/` | PangoCairo text rendering on Linux |
| `cmake/` | CMake helper modules – `bin2h.cmake` (binary→C array) and `generate_scripts_data.cmake` (ECL/SCL embed pipeline) |
| `scripts/` | ECL/SCL script source files – assembled by `script_tool` at build time and embedded as C arrays via `scripts_data.cpp` |
| `tools/` | Build tools – `pack_tool` (DAT/PAK pack manipulation + music data migration) and `script_tool` (ECL/SCL disasm/asm) |
| `libs/` | Vendored Git submodules: SDL3, miniaudio, BLAKE3, dr_libs, libogg, libvorbis, libwebp, tomlplusplus, tinysoundfont |

Entry point: `game/main.cpp`.

## Tooling

- C++23, extensions off
- Formatter: `.clang-format` (LLVM style)
- Linter: `.clang-tidy`

## Resources

The game loads its runtime data from the `bin/` directory at startup:

- `IMAGES.PAK` — bitmap atlas merged from the original `GRAPH.DAT` and `GRAPH2.DAT`
- `MAP.PAK` — stage maps and demo replays repacked from the original 48-entry `ENEMY.DAT`
- `MUSIC.PAK` — BGM tracks with UTF-8 titles/comments (migrated from GBK-encoded `MUSIC.DAT` + `ENEMY.DAT` entries 027–046)
- `SOUND.PAK` — sound effects
- `SSG.TOML` — runtime configuration
- `bgm/`, `soundfonts/` — additional music metadata and soundfont files

PAK files are produced from the original GBK-era assets via `pack_tool` (see
`tools/README.md`).

## License

[MIT](LICENSE)

----

## Original README by pbg

### これは何？

* 西方プロジェクト第一弾 **秋霜玉** のソースコードです。
* コンパイルできるかもしれませんが、すべてのソースコードが含まれているわけではないのでリンクはできません。
* 画像、音楽、効果音、スクリプト等のリソースは含まれません。

### 参考までに

* 基本、開発当時（2000年前後）のままですが、文字コードを utf-8 に変更し、一部コメント（黒歴史ポエム）は削除してあります。インデント等も当時のままなので、読みにくい箇所があるかもしれません。
* 8bit/16bitカラーの混在、MIDI再生関連、浮動小数点数演算を避ける、あたりが懐かしポイントになるかと思います。
* 8.3形式のファイル名が多いのは、PC-98 時代に書いたコードの一部を流用していたためです。
* リソースのアーカイブ展開に関するコードはもろもろの影響を考え、このリポジトリには含めていません。

### たぶん紛失してしまったソースコード

以下のコードについては、見つかり次第追加するかもしれません。

* リソースのアーカイバ
