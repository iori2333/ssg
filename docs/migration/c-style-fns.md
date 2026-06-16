# C 风格函数签名清单

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 一、头文件中的 `(void)` 参数（需删除）

| # | 文件:行 | 当前签名 |
| --- | --- | --- |
| 1 | `core/ENTRY.h:11` | `void XCleanup(void);` |
| 2 | `core/ENTRY.h:26` | `void XGrpTryCycleDisp(void);` |
| 3 | `core/ENTRY.h:27` | `void XGrpTryCycleScMode(void);` |
| 4 | `core/CONFIG.h:184` | `void ConfigSave(void);` |
| 5 | `core/GIAN.h:132` | `void StdStatusOutput(void);` |
| 6 | `core/LOADER.h:90` | `void LoaderInit(void);` |
| 7 | `core/LOADER.h:91` | `void LoaderCleanup(void);` |
| 8 | `core/LOADER.h:106` | `void LoadPaletteFromEnemy(void);` |
| 9 | `core/LOADER.h:109` | `void ReloadGraph(void);` |
| 10 | `core/LOADER.h:113` | `void EnterBombPalette(void);` |
| 11 | `core/LOADER.h:114` | `void LeaveBombPalette(void);` |
| 12 | `gameflow/GAMEMAIN.h:29` | `void GameRestart(void);` |
| 13 | `gameflow/GAMEMAIN.h:31` | `void GameOverInit(void);` |
| 14 | `gameflow/GAMEMAIN.h:32` | `void GameContinue(void);` |
| 15 | `stage/WindowSys.h:315` | `void MWinOpen(void);` |
| 16 | `stage/WindowSys.h:316` | `void MWinClose(void);` |
| 17 | `stage/WindowSys.h:317` | `void MWinForceClose(void);` |
| 18 | `stage/WindowSys.h:318` | `void MWinMove(void);` |
| 19 | `stage/WindowSys.h:319` | `void MWinDraw(void);` |
| 20 | `stage/WindowCtrl.h:27` | `void InitMainWindow(void);` |
| 21 | `stage/WindowCtrl.h:28` | `void InitExitWindow(void);` |
| 22 | `stage/WindowCtrl.h:29` | `void InitContinueWindow(void);` |
| 23 | `effect/EFFECT.h:93` | `void GrpDrawNote(void);` |

## 二、.cpp 文件函数定义上的 `extern`（需删除）

| # | 文件:行 | 当前签名 |
| --- | --- | --- |
| 1 | `core/GIAN.cpp:26` | `extern void StdStatusOutput(void) {` |
| 2 | `gameflow/GAMEMAIN.cpp:103` | `extern bool ScoreNameInit(void) {` |
| 3 | `gameflow/GAMEMAIN.cpp:621` | `extern bool GameNextStage(void) {` |
| 4 | `gameflow/GAMEMAIN.cpp:649` | `extern bool GameReplayInitAll(const char8_t *fn) {` |
| 5 | `gameflow/GAMEMAIN.cpp:854` | `extern void GameRestart(void) {` |
| 6 | `gameflow/GAMEMAIN.cpp:857` | `extern bool GameExit(bool bNeedChgMusic) {` |
| 7 | `gameflow/GAMEMAIN.cpp:904` | `extern void GameOverInit(void) {` |
| 8 | `gameflow/GAMEMAIN.cpp:914` | `extern void GameContinue(void) {` |
| 9 | `core/LOADER.cpp:633` | `extern void LoadPaletteFromEnemy(void) {` |
| 10 | `core/CONFIG.cpp:168` | `extern void ConfigLoad() {` |
| 11 | `core/CONFIG.cpp:177` | `extern void ConfigSave(void) {` |
| 12 | `effect/FONTUTY.cpp:148` | `extern void GrpPut16(int x, int y, const char *s) {` |
| 13 | `effect/FONTUTY.cpp:165` | `extern void GrpPut16c2(...) {` |
| 14 | `effect/FONTUTY.cpp:182` | `extern void GrpPutc(...) {` |
| 15 | `effect/FONTUTY.cpp:190` | `extern void GrpPut57(...) {` |
| 16 | `effect/FONTUTY.cpp:211` | `extern void GrpPut7B(...) {` |
| 17 | `effect/FONTUTY.cpp:231` | `extern void GrpPutScore(...) {` |
| 18 | `effect/FONTUTY.cpp:264` | `extern void GrpPutMidNum(...) {` |

> **注意**：计划仅列出 3 处，实际发现 18 处。Stage 1.1 应全部处理。（`FONTUTY.cpp` 的 `constinit` 变量声明除外）

## 三、`typedef struct` 残留（需改为命名 struct）

| # | 文件:行 | 当前代码 |
| --- | --- | --- |
| 1 | `stage/WindowSys.h:186` | `typedef struct tagWINDOW_SYSTEM { ... } WINDOW_SYSTEM;` |
| 2 | `core/CONFIG.h:169` | `typedef struct tagDEBUG_DATA { ... } DEBUG_DATA;` |
| 3 | `enemy/BOSS.h:38` | `typedef struct tagBOSSHPG_INFO { ... } BOSSHPG_INFO;` |
| 4 | `effect/effect_manager.h:26` | `typedef struct { ... } Stg6Raster;` |
| 5 | `effect/effect_manager.h:27` | `typedef struct { ... } Stg6Star;` |
| 6 | `bullet/LASER.h:83` | `typedef struct{ ... } REFLECTOR;`（注释中） |
| 7 | `core/GIAN.h:113-125` | `typedef struct tagSCORE_DATA` / `tagHIGH_SCORE`（注释中） |
| 8 | `stage/SCROLL.cpp:23` | `typedef struct tagScrollSaveHeader`（.cpp 内部，可后续处理） |
| 9 | `stage/WindowSys.cpp:17` | `typedef struct tagMSG_WINDOW`（.cpp 内部，可后续处理） |

## 四、.cpp 文件中的 `static void xxx(void)`（注意）

以下 .cpp 内部 `static` 函数声明使用了 `(void)`，应当一并清理：

- `stage/SCROLL.cpp:31-52`：~15 个 `static void xxx(void);` 前向声明
- `enemy/BOSS.cpp:53`：`static void HPG_Close(void);`
- `core/CONFIG.cpp:152`：`static void DebugInit(void);`
- `effect/FONTUTY.cpp:58,62`：`TextBackend_GDIInit(void)` / `TextBackend_GDICleanup(void)`

## 验证

```bash
rg "void \w+\(void\)" GIAN07/      # 应返回 0 结果（除保留项）
rg "^extern \w" GIAN07/**/*.cpp    # 应返回 0 结果
rg "typedef struct" GIAN07/        # 应返回 0 结果（除注释外）
```
