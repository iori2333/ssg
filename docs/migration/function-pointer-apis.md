# C 风格函数指针 API 清单

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 概述

项目中有若干 C 风格函数指针，大部分可保留（简单回调），仅 `GameInit` 需要改为 `std::function`。

---

## 需要修改

| # | 文件:行 | 当前签名 | 目标 |
| --- | --- | --- | --- |
| 1 | `gameflow/GAMEMAIN.h:28` | `bool GameInit(void (*NextProc)(bool &quit));` | `[[nodiscard]] bool GameInit(std::function<void(bool &)> next_proc);` |
| 2 | `gameflow/GAMEMAIN.cpp:588` | 同上（实现） | 同上 |

**调用方**（需更新为 lambda）：
| 文件:行 | 当前调用 |
| --- | --- |
| `gameflow/GAMEMAIN.cpp:649` | `GameInit(ReplayProcAll)` |
| `gameflow/GAMEMAIN.cpp:774` | `GameInit(DemoProc)` |

---

## 保留（不修改）

这些函数指针是简单回调或数据结构的一部分，改 `std::function` 会引入不必要的开销。

| # | 文件:行 | 签名 | 理由 |
| --- | --- | --- | --- |
| 1 | `stage/WindowSys.h:79` | `void (*ExCmd)(void);` | 简单回调 |
| 2 | `stage/WindowSys.h:120` | `void (*OptionFn)(int_fast8_t delta) = nullptr;` | 简单回调 |
| 3 | `stage/WindowSys.h:157` | `void (*SetItems)(bool tick) = [](bool) {};` | 已用默认 lambda |
| 4 | `stage/WindowSys.h:163` | `void (*set_items)(bool) = [](bool) {}` | 已用默认 lambda |
| 5 | `stage/WindowSys.h:173` | `constexpr WINDOW_MENU(void (*set_items)(bool), ...)` | 构造函数参数 |
| 6 | `stage/WindowSys.h:215-217` | `void (*Generate)(...)` / `bool (*Handle)(...)` | 菜单回调 |
| 7 | `enemy/BOSS.h:25` | `void (*ExMove)(BossData *);` | 特殊移动函数指针 |
| 8 | `player/MAIDTAMA.cpp:77,87` | `static void (*MaidTamaFunc[4][9])(void)` | 函数指针表，保留 |

---

## 已现代化 ✓

| 文件:行 | 签名 | 说明 |
| --- | --- | --- |
| `core/ENTRY.h:18` | `std::invocable<GRAPHICS_PARAMS &>` | 已用 concept |

---

## 验证

```bash
./build_windows.bat
# 启动游戏 → 标题 → 关卡 → 演示模式 → 重放模式
```
