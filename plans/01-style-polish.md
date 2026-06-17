# Stage 1 — 风格打磨（4 个 PR，机械改动）

## 目标
- 去掉所有 C 风格函数签名的 `(void)`、`.cpp` 内函数定义的 `extern`。
- 删除所有 `// was xxx` 迁移注释。
- 删除注释掉的旧代码。
- 清理 `this->` 冗余和 `Bullets.` 自引用。

**风险等级**：低（纯编辑，语义不变）。

---

## PR 1.1：去除 `(void)` 与 `extern`

### 改动列表

#### 头文件中的 `(void)`（约 23 处）

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `core/ENTRY.h` | 11 | `void XCleanup(void);` | `void XCleanup();` |
| `core/ENTRY.h` | 26 | `void XGrpTryCycleDisp(void);` | `void XGrpTryCycleDisp();` |
| `core/ENTRY.h` | 27 | `void XGrpTryCycleScMode(void);` | `void XGrpTryCycleScMode();` |
| `core/CONFIG.h` | 184 | `void ConfigSave(void);` | `void ConfigSave();` |
| `core/GIAN.h` | 132 | `void StdStatusOutput(void);` | `void StdStatusOutput();` |
| `core/LOADER.h` | 90 | `void LoaderInit(void);` | `void LoaderInit();` |
| `core/LOADER.h` | 91 | `void LoaderCleanup(void);` | `void LoaderCleanup();` |
| `core/LOADER.h` | 92-93 | `bool LoadStageData(uint8_t stage);` `bool LoadGraph(int stage);` | 不变 |
| `core/LOADER.h` | 106 | `void LoadPaletteFromEnemy(void);` | `void LoadPaletteFromEnemy();` |
| `core/LOADER.h` | 109 | `void ReloadGraph(void);` | `void ReloadGraph();` |
| `gameflow/GAMEMAIN.h` | 29 | `void GameRestart(void);` | `void GameRestart();` |
| `gameflow/GAMEMAIN.h` | 31 | `void GameOverInit(void);` | `void GameOverInit();` |
| `gameflow/GAMEMAIN.h` | 32 | `void GameContinue(void);` | `void GameContinue();` |
| `stage/WindowSys.h` | 315-319 | `void MWinOpen(void);` 等 5 个 | 全部去掉 `(void)` |
| `stage/WindowSys.h` | 287-289 | `void CWinMove(WINDOW_SYSTEM *ws);` 等 3 个 | 不变（带参数） |
| `stage/WindowCtrl.h` | 27-29 | `void InitMainWindow(void);` 等 3 个 | 全部去掉 `(void)` |
| `effect/EFFECT.h` | 92-93 | `void GrpDrawSpect(int x, int y);` `void GrpDrawNote(void);` | 第二个去掉 `(void)` |

#### .cpp 文件中的 `extern` 函数定义（3 处）

| 文件 | 行 | 旧 | 新 |
| --- | --- | --- | --- |
| `gameflow/GAMEMAIN.cpp` | 103 | `extern bool ScoreNameInit(void) {` | `bool ScoreNameInit() {` |
| `gameflow/GAMEMAIN.cpp` | 621 | `extern bool GameNextStage(void) {` | `bool GameNextStage() {` |
| `core/GIAN.cpp` | 26 | `extern void StdStatusOutput(void) {` | `void StdStatusOutput() {` |

### 验证

```bash
# 构建
./build_windows.bat
# 必须 0 错误 0 警告
```

```bash
# 二次确认
rg "void \w+\(void\)" GIAN07/   # 应该返回 0 结果
rg "^extern \w" GIAN07/*.cpp GIAN07/*/*.cpp   # 应该返回 0 结果
```

### 不变范围
- 头文件中带参数的 `void f(int x)` 不动。
- 函数指针类型中的 `(void)` 不动（如 `void (*ExCmd)(void)`）。

---

## PR 1.2：删除所有 `// was xxx` 迁移注释

### 改动范围

预计 200+ 处，集中在 Manager 头文件中。

#### 完整列表（按文件）

| 文件 | 行号范围 | 数量（约） |
| --- | --- | --- |
| `bullet/bullet_manager.h` | 24-49 | 26 |
| `enemy/enemy_manager.h` | 38-67 | 30 |
| `enemy/boss_manager.h` | 24-77 | 54 |
| `gameflow/gameflow_manager.h` | 47-56 | 10 |
| `effect/effect_manager.h` | 74-149 | 76 |
| `gameflow/demo_manager.h` | 38-48 | 11 |
| `gameflow/score_manager.h` | 29-35 | 7 |
| `stage/scroll_manager.h` | 18-23 | 6 |
| `player/player_manager.h` | 19-23 | 5 |
| `gameflow/rank_manager.h` | 13-14 | 2 |
| `player/item_manager.h` | 15-19 | 5 |
| `player/MAID.h` | 35-49 | 12（`// evade_add` 等） |
| `enemy/BOSS.h` | 49-50 | 2 |

#### 操作流程

```bash
# 列出所有出现位置
rg -n "// was " GIAN07/ | head -50

# 手动或脚本批量删除：每个 Manager 头文件单独 review，确认无歧义后删除
```

#### 保留范围
设计意图类注释（如 `// 弾の最大発生数`、`// 顔グラ用`）保留。
不删除 `// ==` 或 `// ===` 形式的大区块分隔注释。

### 验证

```bash
rg "// was " GIAN07/   # 应该返回 0 结果
./build_windows.bat    # 编译通过
```

### 历史追溯
迁移历史通过 git log 追溯：

```bash
git log -S "tama_set" -- bullet/bullet_manager.h
git log -S "enemy_move" -- enemy/enemy_manager.h
```

---

## PR 1.3：删除注释掉的旧代码

### 改动列表

| 文件 | 行 | 内容 | 处理 |
| --- | --- | --- | --- |
| `core/GIAN.h` | 112-125 | `SCORE_DATA`、`HIGH_SCORE` typedef 块 | 删除 |
| `bullet/LASER.h` | 81-89 | `REFLECTOR` struct 注释 | 删除 |
| `core/LOADER.h` | 111-115 | `EnterBombPalette` / `LeaveBombPalette` 注释 | 删除 |
| `enemy/ENEMY.cpp` | 81-86 | `inline Debug(...)` 注释 | 删除 |
| `bullet/TAMA.cpp` | 23-29 | 旧 changelog 注释 | 删除 |
| `enemy/ENEMY.cpp` | 19-22 | ECL 命令头注释 | 删除 |
| `enemy/BOSS.cpp` | 185-200 | 注释掉的 `Right[6]`、`Left[6]` 数组 | 删除 |
| `enemy/ENEMY.cpp` | 456-486 | `_EnemyDrawBomb` 内的旧 `switch(count/...)` 注释 | 删除 |
| `gameflow/GAMEMAIN.cpp` | 740-755 | `DemoInit` 内的注释掉分支 | 删除 |
| `gameflow/GAMEMAIN.cpp` | 754-755 | 体验版相关注释 | 删除 |

#### 转移到 `docs/migration/CHANGELOG-extracted.md`
如有价值的设计历史，提取到 `docs/migration/CHANGELOG-extracted.md`。

### 验证

```bash
./build_windows.bat
```

---

## PR 1.4：清理 `this->` 冗余和 `Bullets.` 自引用（拆分多个子 PR）

### 总体策略

- 简单读写：`this->xxx` → `xxx`。
- 保留：参数与成员同名（消歧义）、限定符调用（如 `BossManager::STDMove`）。
- 工具：`clang-tidy --checks=readability-redundant-this --fix`。

### 子 PR 1.4.A：`bullet/TAMA.cpp` 的 `Bullets.` 自引用

**改动**：`BulletManager` 方法体内部所有 `Bullets.xxx` 改为裸成员名。

**具体行号**（来自盘点）：
- 行 53, 55, 61, 71, 75, 77, 127, 131, 133, 196, 205, 207, 255, 279, 283, 307, 325, 428, 549, 559, 576, 586, 590, 600, 618, 634, 638, 654, 666, 667, 671, 673, 768, 790

**操作流程**：
1. 在 `TAMA.cpp` 顶部确认所有 `BulletManager::xxx` 方法。
2. 方法体内部 `Bullets.` 全部替换为裸名（`Bullets.speed` → `speed`）。
3. 验证编译。
4. 提交。

### 子 PR 1.4.B：`enemy/BOSS.cpp` 的 `this->` 清理

**改动**：`BossManager` 方法体内部所有 `this->xxx` 简单读写替换。

**保留**：
- `this->STDMove(b)` 这类不存在（已用 `BossManager::STDMove` 限定符）。
- `auto *b = &it;`（已经是局部变量）。

### 子 PR 1.4.C：`enemy/ENEMY.cpp` 的 `this->` 清理

**改动**：`EnemyManager` 方法体内部所有 `this->xxx` 简单读写替换。

**保留**：
- `this->UpdateHoming(e)`（同文件内部成员，OK 可保留也可去）。
- 参数与成员同名（如 `void InitIndices(void)` 无参数，无歧义）。

### 子 PR 1.4.D：`stage/ENDING.cpp` + `gameflow/*` 的 `this->` 清理

**改动**：
- `stage/ENDING.cpp`：约 100+ 处 `this->`。
- `gameflow/SCORE.cpp`、`gameflow/PRankCtrl.cpp`、`gameflow/DEMOPLAY.cpp`、`gameflow/ENDING.cpp`：相应位置。

### 子 PR 1.4.E：其他文件

**范围**：
- `bullet/LASER.cpp`、`bullet/LLASER.cpp`、`bullet/HOMINGL.cpp`
- `enemy/EnemyExCtrl.cpp`
- `effect/EFFECT.cpp`、`effect/EFFECT3D.cpp`、`effect/FRAGMENT.cpp`、`effect/BOMBEFC.cpp`
- `player/MAID.cpp`、`player/MAIDTAMA.cpp`、`player/ITEM.cpp`
- `stage/SCROLL.cpp`、`stage/MUSIC.cpp`、`stage/WindowSys.cpp`、`stage/WindowCtrl.cpp`

### 验证（每个子 PR）

```bash
./build_windows.bat
# 0 错误 0 警告
```

---

## 阶段 1 总体验证

构建：
```bash
./build_windows.bat
```

警告数对比：
- 基线（阶段 0）：记录当前警告数。
- 阶段 1 后：警告数应 ≤ 基线（不应增加）。

游戏冒烟测试：
- 进入标题画面
- 开始游戏（任意一关）
- 触发敌人出现
- 触发子弹发射
- 触发 boss 出现
- 通关一关
- 视觉、操控、声音无异常

## 提交策略

每个子 PR 单独 review + 合并，避免大批量改动混入。

## 风险登记表

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 删除 `// was xxx` 误删设计意图注释 | 信息丢失 | 区分两类注释：`// was`（迁移痕迹） vs `// xxx`（设计意图） |
| `this->` 误改导致编译失败 | 编译错误 | 逐文件编译验证 |
| `Bullets.` 自引用误改访问路径 | 行为变化 | 编译 + 冒烟测试 |
| `extern` 移除影响外部链接 | 链接错误 | 仅在 .cpp 内的函数定义处去 `extern`；头文件中的 `extern` 声明保留 |
