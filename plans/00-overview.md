# Stage 0 — 盘点与基线

## 目标
为后续 6 个阶段建立可验证的事实基线：清单文件 + 基线构建结果。

## 任务清单

### 0.1 创建迁移文档目录
- 创建 `docs/migration/` 目录。
- 在 `docs/migration/` 下创建以下清单文件。

### 0.2 输出 7 份清单

| 文件 | 内容 | 命令 |
| --- | --- | --- |
| `8.3-filenames.md` | 27 对文件的旧名 → 新名映射表 | 手写（基于 0.1 调研） |
| `using-aliases.md` | 28 个 `using` 别名 + 全局调用点统计 | `rg -c "TAMA_DATA" GIAN07/` 等 |
| `was-comments.md` | `// was xxx` 注释位置 + 数量 | `rg -n "// was " GIAN07/ \| wc -l` |
| `this-arrow-usage.md` | `this->` 与 `Bullets.` 自引用统计 | `rg -c "this->" GIAN07/ --type cpp` |
| `c-style-fns.md` | `(void)` / `extern` / `typedef struct` 位置 | `rg -n "void \w+\(void\)" GIAN07/` 等 |
| `indsort-calls.md` | `Indsort` 调用点 | `rg -n "Indsort" GIAN07/` |
| `function-pointer-apis.md` | C 风格函数指针 API | `rg -n "void \(\*" GIAN07/` |

### 0.3 调研：8.3 文件名重命名映射

| 旧名 | 新名 | 子目录 |
| --- | --- | --- |
| `core/GIAN.cpp/h` | `core/gian.cpp/h` | core/ |
| `core/GIAN07.rc` | `core/gian07.rc` | core/ |
| `core/ENTRY.cpp/h` | `core/entry.cpp/h` | core/ |
| `core/EXDEF.h` | `core/point.h` | core/ |
| `core/LEVEL.h` | `core/level.h` | core/ |
| `core/LOADER.cpp/h` | `core/loader.cpp/h` | core/ |
| `core/CONFIG.cpp/h` | `core/config.cpp/h` | core/ |
| `core/LZ_UTY.cpp/h` | `core/lz_uty.cpp/h` | core/ |
| `bullet/TAMA.cpp/h` | `bullet/bullet.cpp/h` | bullet/ |
| `bullet/LASER.cpp/h` | `bullet/laser.cpp/h` | bullet/ |
| `bullet/LLASER.cpp/h` | `bullet/long_laser.cpp/h` | bullet/ |
| `bullet/HOMINGL.cpp/h` | `bullet/homing_laser.cpp/h` | bullet/ |
| `enemy/ENEMY.cpp/h` | `enemy/enemy.cpp/h` | enemy/ |
| `enemy/BOSS.cpp/h` | `enemy/boss.cpp/h` | enemy/ |
| `effect/EFFECT.cpp/h` | `effect/effect.cpp/h` | effect/ |
| `effect/EFFECT3D.cpp/h` | `effect/effect3d.cpp/h` | effect/ |
| `player/MAID.cpp/h` | `player/player.cpp/h` | player/ |
| `player/MAIDTAMA.cpp/h` | `player/player_shot.cpp/h` | player/ |
| `stage/SCROLL.cpp/h` | `stage/scroll.cpp/h` | stage/ |
| `stage/SCL.h` | `stage/scene.h` | stage/ |
| `gameflow/GAMEMAIN.cpp/h` | `gameflow/game_main.cpp/h` | gameflow/ |
| `gameflow/ENDING.cpp/h` | `gameflow/ending.cpp/h` | gameflow/ |
| `gameflow/DEMOPLAY.cpp/h` | `gameflow/demo_play.cpp/h` | gameflow/ |
| `gameflow/PRankCtrl.cpp/h` | `gameflow/play_rank.cpp/h` | gameflow/ |
| `gameflow/SCORE.cpp/h` | `gameflow/score.cpp/h` | gameflow/ |
| `stage/WINDOWSYS.cpp/h` | `stage/window_sys.cpp/h` | stage/ |
| `stage/WINDOWCTRL.cpp/h` | `stage/window_ctrl.cpp/h` | stage/ |

**大小写冲突风险**：
- Windows 文件系统默认不区分大小写
- Linux CI 会区分
- `EXDEF.h → point.h` 需要先 grep `point.h` 确认无冲突

### 0.4 调研：28 个 using 别名

**批次 A（20 个，低影响面）**：
```
FACE_DATA → FaceData
ENDING_GRP → EndingGrp
HLaserData → HomingLaserData
HLaserInfo → HomingLaserInfo
EXHITCHK → ExHitCheck
INT_VECTOR → InterruptVector
MAID → Player
BOSS_DATA → BossData
FRAGMENT_DATA → FragmentData
DEMOPLAY_INFO → DemoPlayState
SCROLL_INFO → ScrollState
SCL_INFO → SceneState
CIRCLE_EFC_DATA → CircleEffectData
SEFFECT_DATA → StringEffectData
LOCKON_INFO → LockOnInfo
SCREENEFC_INFO → ScreenEffectState
BIT_PARAM → BitParam
BIT_DATA → BitData
NR_NAME_DATA → NrNameData
NR_SCORE_DATA → NrScoreData
NR_SCORE_STRING → NrScoreString
```

**批次 B（5 个，大影响面）**：
```
TAMA_CMD → BulletCommand       (TAMA.h:149)
TAMA_DATA → Bullet             (TAMA.h:150)
LASER_CMD → LaserCommand       (LASER.h:52)
LLASER_CMD → LongLaserCommand  (LLASER.h:48)
LLASER_DATA → LongLaserData    (LLASER.h:75)
ENEMY_DATA → EnemyData         (ENEMY.h:101)
ITEM_DATA → ItemData           (ITEM.h:35)
```

### 0.5 调研：函数指针 API

| 位置 | 当前签名 | 目标签名 |
| --- | --- | --- |
| `gameflow/GAMEMAIN.h:28` | `bool GameInit(void (*NextProc)(bool &quit))` | `bool GameInit(std::function<void(bool&)> next_proc)` |
| `stage/WindowSys.h:117` | `bool (*CallBackFn)(INPUT_BITS)` | 保留（简单回调） |
| `stage/WindowSys.h:120` | `void (*OptionFn)(int_fast8_t)` | 保留 |
| `stage/WindowSys.h:157` | `void (*SetItems)(bool)` | 保留 |
| `stage/WindowSys.h:163` | `void (*set_items)(bool)` | 保留 |
| `stage/WindowSys.h:215-216` | `void (*Generate)(WINDOW_CHOICE&, size_t, size_t)` | 保留 |
| `stage/WindowSys.h:217` | `bool (*Handle)(INPUT_BITS, size_t)` | 保留 |
| `stage/WindowSys.h:79` | `void (*ExCmd)(void)` | 保留 |
| `core/ENTRY.h:18` | `std::invocable<GRAPHICS_PARAMS &>` | 已用 concept ✓ |

**决策**：仅改 `GameInit`，其他回调函数指针保持简单签名。

### 0.6 调研：`Indsort` 调用点

| 位置 | 状态 |
| --- | --- |
| `core/entity.h:14-45` | 通用模板（带 `ShouldDelete` 谓词） |
| `bullet/TAMA.h:171-176` | 包装 `Indsort<TAMA_DATA, N>` |
| `enemy/ENEMY.cpp:46-51` | 包装 `Indsort<EnemyData, N>` |
| `player/ITEM.cpp` | 推测有调用，待确认 |
| `effect/FRAGMENT.cpp` | 推测有调用，待确认 |

### 0.7 调研：`typedef struct` 残留

| 位置 | 旧 | 新 |
| --- | --- | --- |
| `stage/WindowSys.h:186-209` | `typedef struct tagWINDOW_SYSTEM { ... } WINDOW_SYSTEM;` | `struct WINDOW_SYSTEM { ... };` |
| `core/CONFIG.h:169-175` | `typedef struct tagDEBUG_DATA { ... } DEBUG_DATA;` | `struct DebugData { ... };` |
| `enemy/BOSS.h:38-46` | `typedef struct tagBOSSHPG_INFO { ... } BOSSHPG_INFO;` | `struct BossHpgInfo { ... };` |
| `effect/effect_manager.h:26-27` | `typedef struct { int x, y; char vy; ... } Stg6Raster;` | `struct Stg6Raster { ... };` |
| `bullet/LASER.h:83` | 注释中的 `REFLECTOR` | 删除注释 |
| `core/GIAN.h:113-125` | 注释中的 `SCORE_DATA` / `HIGH_SCORE` | 删除注释 |

### 0.8 调研：C 风格函数签名

**需要去掉 `(void)`**（约 23 处）：
- `core/ENTRY.h:11` `void XCleanup(void);`
- `core/ENTRY.h:26-27` `XGrpTryCycleDisp(void)` / `XGrpTryCycleScMode(void)`
- `core/CONFIG.h:184` `void ConfigSave(void);`
- `core/GIAN.h:132` `void StdStatusOutput(void);`
- `core/LOADER.h:90-114`（含多个）
- `gameflow/GAMEMAIN.h:29,31,32,40`
- `stage/WindowSys.h:315-319, 287-326`
- `stage/WindowCtrl.h:27-29`
- `effect/EFFECT.h:92-93`

**需要去掉 `extern`（函数定义处）**（3 处）：
- `gameflow/GAMEMAIN.cpp:103` `extern bool ScoreNameInit(void) {`
- `gameflow/GAMEMAIN.cpp:621` `extern bool GameNextStage(void) {`
- `core/GIAN.cpp:26` `extern void StdStatusOutput(void) {`

### 0.9 调研：`// was xxx` 注释位置

预计 200+ 处，主要集中在 Manager 头文件中：
- `bullet/bullet_manager.h:24-49`（26 处）
- `enemy/enemy_manager.h:38-67`（30 处）
- `enemy/boss_manager.h:24-77`（54 处）
- `gameflow/gameflow_manager.h:47-56`（10 处）
- `effect/effect_manager.h:74-149`（76 处）
- `gameflow/demo_manager.h:38-48`（11 处）
- `gameflow/score_manager.h:29-35`（7 处）
- `stage/scroll_manager.h:18-23`（6 处）
- `player/player_manager.h:19-23`（5 处）
- `gameflow/rank_manager.h:13-14`（2 处）
- `player/item_manager.h:15-19`（5 处）

### 0.10 调研：`this->` 与 `Bullets.` 自引用

**`this->` 出现最多的文件**：
- `boss_manager.cpp`、`enemy/ENEMY.cpp`（1200+ 行）、`bullet/TAMA.cpp`（1200 行）
- `stage/ENDING.cpp`、`gameflow/SCORE.cpp`、`gameflow/PRankCtrl.cpp`

**`Bullets.` 自引用出现位置**（`bullet/TAMA.cpp` 内 `BulletManager::xxx` 方法体）：
- 行 53, 55, 61, 71, 75, 77, 127, 131, 133, 196, 205, 207, 255, 279, 283, 307, 325, 428, 549, 559, 576, 586, 590, 600, 618, 634, 638, 654, 666, 667, 671, 673, 768, 790

### 0.11 构建基线

```bash
./build_windows.bat
```

**预期**：成功构建，记录当前警告数、错误数、生成时间。

**验证项**：
- 0 个编译错误
- 警告数记录（作为后续 PR 的对比基准）
- 链接成功
- 输出 `build/bin/GIAN07.exe`

### 0.12 EclInterpreter 使用点调研

```bash
rg -n "EclInterpreter" GIAN07/
```

**预期**：仅在 `ecl_interpreter.h/cpp` 自身 + 少数使用点（需找出）。

## 产出物
- 7 份 Markdown 清单文件（在 `docs/migration/`）
- `docs/migration/rename-mapping.md`（重命名映射表）
- `docs/migration/using-aliases.md`（28 个别名详情）
- `docs/migration/baseline-build.md`（基线构建结果）

## 验证标准
- 所有清单文件存在且内容完整
- 基线构建成功
- 清单中所有行号准确（验证：抽样 5 处用 `rg` 复核）

## 不提交
本阶段是调研性质，不修改任何源代码。
