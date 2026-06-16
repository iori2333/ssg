# Stage 2 — 类型系统修复（2 个 PR）

## 目标
- `typedef struct` 残留 → 命名 `struct`。
- `uint8_t = 0/0xff` 标志位 → `bool`。
- 加 `[[nodiscard]]` / `noexcept` / `const` 传播。
- 类型精度修正（如 `unsigned int` → 命名类型）。

**风险等级**：低-中。

---

## PR 2.1：`typedef struct` → `struct`

### 改动列表

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `stage/WindowSys.h` | 186-209 | `typedef struct tagWINDOW_SYSTEM { ... } WINDOW_SYSTEM;` | `struct WINDOW_SYSTEM { ... };` |
| `core/CONFIG.h` | 169-175 | `typedef struct tagDEBUG_DATA { ... } DEBUG_DATA;` | `struct DebugData { ... };` |
| `enemy/BOSS.h` | 38-46 | `typedef struct tagBOSSHPG_INFO { ... } BOSSHPG_INFO;` | `struct BossHpgInfo { ... };` |
| `effect/effect_manager.h` | 26-27 | `typedef struct { int x, y; char vy; ... } Stg6Raster;` | `struct Stg6Raster { int x, y; char vy; ... };` |
| `effect/effect_manager.h` | 27 | `typedef struct { int x, y; int vy; } Stg6Star;` | `struct Stg6Star { int x, y; int vy; };` |
| `bullet/LASER.h` | 83 | 注释中的 `typedef struct{ ... } REFLECTOR;` | 删除注释 |
| `core/GIAN.h` | 113-125 | 注释中的 `typedef struct tagSCORE_DATA` | 删除注释 |

### 同步更新（外部引用方）

#### `WINDOW_SYSTEM` 相关

`stage/WindowCtrl.h:19-24` 当前：
```cpp
extern struct tagWINDOW_SYSTEM MainWindow;
extern struct tagWINDOW_SYSTEM BGMPackWindow;
extern struct tagWINDOW_SYSTEM ExitWindow;
extern struct tagWINDOW_SYSTEM ContinueWindow;
extern struct tagWINDOW_SYSTEM GameOverSaveWindow;
extern struct tagWINDOW_SYSTEM ReplayFilesWindow;
```

改为：
```cpp
extern struct WINDOW_SYSTEM MainWindow;
extern struct WINDOW_SYSTEM BGMPackWindow;
// ... 其余 4 个
```

#### `DebugData` 相关

`core/CONFIG.h:177` 当前：
```cpp
extern DEBUG_DATA DebugDat;
```

改为：
```cpp
extern DebugData debug_data;
```

**注意**：变量名 `DebugDat` 也应改为 snake_case `debug_data`，但这是更大改动（需扫描所有 `DebugDat.xxx` 使用点）。本 PR 仅改类型，变量名后续可改。

#### `BossHpgInfo` 相关

`enemy/boss_manager.h:15` 当前：
```cpp
BOSSHPG_INFO hpg;  // BossHPG
```

改为：
```cpp
BossHpgInfo hpg;
```

**警告**：这里成员名 `hpg` 也要 review。是否同时改 `hpg` → `hp_gauge`？本 PR 不改（最小改动原则）。

#### `Stg6Raster` / `Stg6Star` 相关

需要扫描所有 `Stg6Raster`、`Stg6Star` 使用点（应在 `effect/EFFECT3D.cpp` 及其使用方）。

### 验证

```bash
./build_windows.bat
# 0 错误
```

```bash
rg "typedef struct" GIAN07/   # 应该返回 0 结果（除注释外）
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `tagWINDOW_SYSTEM` 改名为 `WINDOW_SYSTEM` 引起头文件循环 | 编译失败 | 检查所有 `#include "WindowSys.h"` 顺序 |
| 旧名（如 `DebugDat`）与新类型不匹配 | 链接失败 | 全局 grep 验证 |
| `Stg6Raster` 匿 struct 改命名 struct 后类型兼容性 | 二进制布局变化 | 加 `static_assert(sizeof(Stg6Raster) == ...)` |

---

## PR 2.2：标志位 / 类型精度 / `[[nodiscard]]` / `noexcept` / `const` 传播

### 子任务 2.2.A：`uint8_t = 0/0xff` 改为 `bool`

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `enemy/BOSS.h` | 29 | `uint8_t IsUsed;` | `bool IsUsed = false;` |
| `enemy/BOSS.cpp` | 62 | `it.IsUsed = 0;` | `it.IsUsed = false;` |
| `enemy/BOSS.cpp` | 98 | `this->bosses[n].IsUsed = 0xff;` | `this->bosses[n].IsUsed = true;` |
| `enemy/BOSS.cpp` | 127 | `this->bosses[n].IsUsed = 0xff;` | `this->bosses[n].IsUsed = true;` |
| `enemy/BOSS.cpp` | 151 | `if (b->IsUsed) {` | 不变（bool 上下文 OK） |

**注意**：未来如果 `IsUsed` 还需要位字段（多状态），改回 `uint8_t`。当前 `IsUsed` 只有两种状态（`0` / `非 0`），`bool` 合适。

### 子任务 2.2.B：类型精度修正

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `effect/effect_manager.h` | 39 | `unsigned int mtitle_rect = 0;  // TEXTRENDER_RECT_ID` | `TEXTRENDER_RECT_ID mtitle_rect = {};` |

### 子任务 2.2.C：加 `[[nodiscard]]`

#### 初始化 / 加载函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `core/ENTRY.h` | 10 | `bool XInit(void);` → `[[nodiscard]] bool XInit();` |
| `core/LOADER.h` | 92 | `bool LoadStageData(uint8_t stage);` → `[[nodiscard]]` |
| `core/LOADER.h` | 94 | `bool LoadGraph(int stage);` → `[[nodiscard]]` |
| `core/LOADER.h` | 95 | `bool LoadFace(uint8_t FaceID, uint8_t FileNo);` → `[[nodiscard]]` |
| `core/LOADER.h` | 96 | `bool LoadMusic(unsigned int id);` → `[[nodiscard]]` |
| `core/LOADER.h` | 97 | `bool LoadMusicByHash(const HASH &hash);` → `[[nodiscard]]` |
| `core/LOADER.h` | 98 | `bool LoadMIDIBuffer(BYTE_BUFFER_OWNED);` → `[[nodiscard]]` |
| `core/LOADER.h` | 99 | `bool LoadSound(void);` → `[[nodiscard]]` |

#### 游戏流程函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `gameflow/GAMEMAIN.h` | 28 | `bool GameInit(std::function<void(bool&)>);` → `[[nodiscard]]` |
| `gameflow/GAMEMAIN.h` | 30 | `bool GameExit(bool bNeedChgMusic);` → `[[nodiscard]]` |
| `gameflow/GAMEMAIN.h` | 36 | `bool SProjectInit(void);` → `[[nodiscard]]` |
| `gameflow/GAMEMAIN.h` | 38 | `bool GameExstgInit(void);` → `[[nodiscard]]` |
| `gameflow/GAMEMAIN.h` | 41 | `bool ScoreNameInit(void);` → `[[nodiscard]]` |
| `gameflow/GAMEMAIN.h` | 43 | `bool GameNextStage(void);` → `[[nodiscard]]` |

#### 状态机 / 演示函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `gameflow/demo_manager.h` | 38 | `void Init();` |
| `gameflow/demo_manager.h` | 39 | `bool HasRecordedStages();` → `[[nodiscard]]` |
| `gameflow/demo_manager.h` | 40 | `void FlushStage();` |
| `gameflow/demo_manager.h` | 41 | `bool LoadSetup();` → `[[nodiscard]]` |
| `gameflow/demo_manager.h` | 42 | `bool Record(INPUT_BITS key);` → `[[nodiscard]]` |
| `gameflow/demo_manager.h` | 44 | `bool LoadDemo(int stage);` → `[[nodiscard]]` |
| `gameflow/demo_manager.h` | 47 | `bool LoadReplayAll(const char8_t *fn);` → `[[nodiscard]]` |

#### 分数 / 排名函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `gameflow/score_manager.h` | 29 | `uint8_t SetScoreString(NR_NAME_DATA *, uint8_t);` → `[[nodiscard]]` |
| `gameflow/score_manager.h` | 32 | `uint8_t IsHighScore(const NR_NAME_DATA *, uint8_t);` → `[[nodiscard]]` |
| `gameflow/score_manager.h` | 35 | `bool SaveScoreData(NR_NAME_DATA *, uint8_t);` → `[[nodiscard]]` |

#### Boss / Enemy 函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `enemy/boss_manager.h` | 39 | `bool ApplyDamage(BOSS_DATA&, ENEMY_DATA&, int);` → `[[nodiscard]]` |
| `enemy/boss_manager.h` | 40-42 | `bool DamageAt / DamageAt2 / DamageAt3` → `[[nodiscard]]` |
| `enemy/enemy_manager.h` | 50 | `bool ApplyDamage(EnemyData&, int);` → `[[nodiscard]]` |
| `enemy/enemy_manager.h` | 51-52 | `bool DamageAt / DamageAt2` → `[[nodiscard]]` |

#### Laser 函数

| 文件 | 行 | 函数 |
| --- | --- | --- |
| `bullet/laser_manager.h` | 36 | `int SpawnLong(uint16_t* ind);` → `[[nodiscard]]` |
| `bullet/laser_manager.h` | 58 | `bool SpawnLongLaser(uint8_t id);` → `[[nodiscard]]` |

### 子任务 2.2.D：加 `noexcept`

#### 纯算术 / 状态 setter

| 文件 | 行 | 函数 | 备注 |
| --- | --- | --- | --- |
| `gameflow/rank_manager.h` | 13 | `void Add(int n);` | `noexcept` |
| `gameflow/rank_manager.h` | 14 | `void Reset();` | `noexcept` |
| `bullet/bullet_manager.h` | 36 | `uint8_t Dir(uint16_t i);` | `noexcept` |
| `bullet/bullet_manager.h` | 37 | `int NewSpeed(uint16_t i);` | `noexcept` |
| `bullet/bullet_manager.h` | 38 | `int LineCmdNewSpeed(uint16_t i);` | `noexcept` |
| `bullet/bullet_manager.h` | 39 | `int Speed(uint16_t i);` | `noexcept` |
| `bullet/bullet_manager.h` | 40 | `uint8_t Flag();` | `noexcept` |
| `core/GIAN.h` | 98 | `inline short SPEEDM(uint8_t v);` | `noexcept` |
| `core/GIAN.h` | 99 | `inline short WAVESP(uint8_t v);` | `noexcept` |
| `core/GIAN.h` | 102 | `inline int GX_RND();` | `noexcept` |
| `core/GIAN.h` | 104 | `inline int GY_RND();` | `noexcept` |
| `core/GIAN.h` | 107 | `inline bool HITCHK(int a, int b, int h);` | `noexcept` |

**注意**：加 `noexcept` 前 audit 内部是否有 `throw` 或 `new`。如有，**不能**加 `noexcept`。

### 子任务 2.2.E：`const` 传播

#### 只读指针改为 `const T*`

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `enemy/enemy_manager.h` | 62 | `void UpdateAnimation(EnemyData *e);` | `void UpdateAnimation(const EnemyData *e);` |
| `enemy/enemy_manager.h` | 62 实现 | 同上 | 同上 |
| `bullet/laser_manager.h` | 59-66 | `OpenLong(const EnemyData* e, ...)` 等已 const | 不变 |
| `effect/effect_manager.h` | 94 | `void LockOn(int* x, int* y, int wx64, int hx64);` | 保持（`int*` 引用修改） |
| `enemy/boss_manager.h` | 56 | `void SnakyDelete(const BOSS_DATA *b);` | 已 const ✓ |
| `enemy/ENEMY.h` | 96 | `void Draw() const;` | 已 const ✓ |

#### Const 成员函数

| 文件 | 行 | 候选 | 决策 |
| --- | --- | --- | --- |
| `enemy/ENEMY.h` | 97 | `void UpdateAnimation();` | 调用 `Enemies.UpdateAnimation(this)`，需要 EnmData 上的成员可写。检查后决定是否 const。 |

### 子任务 2.2.F：枚举化魔数

将 `0x0f`、`0xf0`、`0xc0` 等掩码改为命名 `enum` 或 `inline constexpr`：

| 文件 | 行 | 魔数 | 命名建议 |
| --- | --- | --- | --- |
| `bullet/TAMA.cpp` | 74, 130, 204, 343, 352, 384, 386, 398, 399 | `0xf0`, `0x0f`, `0xc0` | 已有 `TAMA_SMALL` 等，扩展为 `enum class` |
| `enemy/ENEMY.cpp` | 357, 1100+ | `0xff`, `0x40`, `0x80` | 已用 `EF_DELETE` 等 |

**决策**：本 PR 不动魔数（统一改为 `enum class` 是更大改动）。仅在已有命名前提下使用。

### 验证

```bash
./build_windows.bat
# 0 错误 0 新警告

# 静态分析
clang-tidy --checks=bugprone-unchecked-optional-access,bugprone-use-after-move GIAN07/...
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `[[nodiscard]]` 强制调用方检查返回值 | 链接失败（如果有调用方忽略返回值） | 全局 grep 调用点，加 `[[maybe_unused]]` 或处理返回值 |
| `noexcept` 内部有 throw | 运行时崩溃 | audit 内部代码 |
| `const` 误改导致 const 违例 | 编译失败 | 逐函数审查 |
| `bool IsUsed` 改为 `bool` 后位运算 | 语义变化 | 当前无位运算，但需 grep 确认 |

### 提交

- 1 个 PR（含所有子任务 2.2.A-F），commit 拆分：
  - commit 1: 子任务 2.2.A + 2.2.B（类型精度 + 标志位）
  - commit 2: 子任务 2.2.C（`[[nodiscard]]`）
  - commit 3: 子任务 2.2.D（`noexcept`）
  - commit 4: 子任务 2.2.E（`const` 传播）
- 每个 commit 独立 review。
