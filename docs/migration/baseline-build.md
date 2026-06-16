# 基线构建报告

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 构建结果

| 项目 | 结果 |
| --- | --- |
| **状态** | ✅ 成功 |
| **错误** | 0 |
| **生成工具** | Visual Studio 18 2026 (MSVC) |
| **配置** | Release |
| **输出** | `build/bin/Release/GIAN07.exe` |

---

## GIAN07 项目警告（14 处）

全部是 MSVC `strcpy` 废弃警告（`-Wdeprecated-declarations`），无功能性 bug：

### effect/EFFECT.cpp（2 处）

| 行 | 内容 |
| --- | --- |
| 354 | `'strcpy' is deprecated` |
| 374 | `'strcpy' is deprecated` |

### gameflow/SCORE.cpp（2 处）

| 行 | 内容 |
| --- | --- |
| 75 | `'strcpy' is deprecated` |
| 236 | `'strcpy' is deprecated` |

### gameflow/GAMEMAIN.cpp（2 处）

| 行 | 内容 |
| --- | --- |
| 399 | `'strcpy' is deprecated` |
| 403 | `'strcpy' is deprecated` |

### stage/WindowCtrl.cpp（8 处）

| 行 | 内容 |
| --- | --- |
| 725 | `'strcpy' is deprecated` |
| 727 | `'strcpy' is deprecated` |
| 1022 | `'strcpy' is deprecated` |
| 1054 | `'strcpy' is deprecated` |
| 1082 | `'strcpy' is deprecated` |
| 1087 | `'strcpy' is deprecated` |
| 1181 | `'strcpy' is deprecated` |
| 1185 | `'strcpy' is deprecated` |

---

## 第三方/平台代码警告（不在 GIAN07 重构范围内）

| 来源 | 数量 | 类型 |
| --- | --- | --- |
| `libs/libvorbis/` | 9 | `strcpy`/`strcat`/`fopen` deprecated |
| `game/debug.cpp` | 1 | `#pragma message`（版本声明） |
| `game/ut_math.cpp` | 1 | `#pragma message`（版本声明） |
| `game/midi.cpp` | 2 | `#pragma message` + `-Wswitch` |
| `platform/sdl/graphics_sdl.cpp` | 1 | `-Wformat`（char8_t） |
| `platform/windows/midi_backend_winmm.cpp` | 1 | `-Wtautological-constant-out-of-range-compare` |

---

## 对比基准

后续每个 Stage 完成后，GIAN07 警告数应与当前基线对比：
- **基线 GIAN07 警告数**：14（全部 strcpy deprecated）
- **目标**：每个 Stage 后 GIAN07 警告数 ≤ 14（不引入新警告）

---

## 完整构建日志

保存于 `build_baseline.log`（项目根目录）。
