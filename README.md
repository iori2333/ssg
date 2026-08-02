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
| `GIAN07/` | Original pbg game code |
| `game/` | Modern platform-independent layer (graphics, audio, input abstractions) |
| `platform/` | Platform backend interfaces |
| `platform/sdl/` | SDL3 backend (Windows and Linux) |
| `platform/windows/` | Win32-native backends (GDI text, WinMM MIDI, file I/O) |
| `platform/c/` | Standard-library fallback implementations |
| `platform/miniaudio/` | Audio backend |
| `platform/linux/` | Linux text rendering |
| `libs/` | Vendored Git submodules (SDL3, miniaudio, libwebp) |

Entry point: `platform/sdl/main.cpp`.

## Tooling

- C++23, extensions off
- Formatter: `.clang-format` (LLVM style)
- Linter: `.clang-tidy`

## Resources

This repository contains source code only. Image, music, sound effect, and script resources are not included.

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
