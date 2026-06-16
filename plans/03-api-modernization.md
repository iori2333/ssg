# Stage 3 — API 现代化（5 个 PR）

## 目标
- 一次性删除 28 个 `using` 后向兼容别名（按 A/B 两批）。
- `GameInit` 函数指针 → `std::function`（保留 `current_state` 字段）。
- 删除 `EclInterpreter` 包装类。
- `Indsort` → `std::ranges::stable_partition`（保留 `indices[]` 间接访问）。

**风险等级**：中。

---

## PR 3.1.A：删除批次 A 的 20 个 `using` 别名（低影响面）

### 别名清单（21 个）

| 别名 | 替换为 | 位置 |
| --- | --- | --- |
| `FACE_DATA` | `FaceData` | `core/LOADER.h:80` |
| `ENDING_GRP` | `EndingGrp` | `core/LOADER.h:87` |
| `HLaserData` | `HomingLaserData` | `bullet/HOMINGL.h:40` |
| `HLaserInfo` | `HomingLaserInfo` | `bullet/HOMINGL.h:53` |
| `EXHITCHK` | `ExHitCheck` | `enemy/BOSS.h:18` |
| `INT_VECTOR` | `InterruptVector` | `enemy/ENEMY.h:102` |
| `MAID` | `Player` | `player/player_types.h:76` |
| `BOSS_DATA` | `BossData` | `enemy/BOSS.h:31` |
| `FRAGMENT_DATA` | `FragmentData` | `effect/FRAGMENT.h:32` |
| `DEMOPLAY_INFO` | `DemoPlayState` | `gameflow/DEMOPLAY.h:39` |
| `SCROLL_INFO` | `ScrollState` | `stage/SCROLL.h:90` |
| `SCL_INFO` | `SceneState` | `stage/SCROLL.h:91` |
| `CIRCLE_EFC_DATA` | `CircleEffectData` | `effect/EFFECT.h:61` |
| `SEFFECT_DATA` | `StringEffectData` | `effect/EFFECT.h:73` |
| `LOCKON_INFO` | `LockOnInfo` | `effect/EFFECT.h:82` |
| `SCREENEFC_INFO` | `ScreenEffectState` | `effect/EFFECT.h:88` |
| `BIT_PARAM` | `BitParam` | `enemy/EnemyExCtrl.h:62` |
| `BIT_DATA` | `BitData` | `enemy/EnemyExCtrl.h:89` |
| `NR_NAME_DATA` | `NrNameData` | `gameflow/SCORE.h:24` |
| `NR_SCORE_DATA` | `NrScoreData` | `gameflow/SCORE.h:33` |
| `NR_SCORE_STRING` | `NrScoreString` | `gameflow/SCORE.h:46` |

### 操作流程

```bash
# 1. 列出所有调用点
for alias in FACE_DATA ENDING_GRP HLaserData HLaserInfo EXHITCHK INT_VECTOR \
            MAID BOSS_DATA FRAGMENT_DATA DEMOPLAY_INFO SCROLL_INFO SCL_INFO \
            CIRCLE_EFC_DATA SEFFECT_DATA LOCKON_INFO SCREENEFC_INFO BIT_PARAM \
            BIT_DATA NR_NAME_DATA NR_SCORE_DATA NR_SCORE_STRING; do
  echo "=== $alias ==="
  rg -l "\b$alias\b" GIAN07/
done
```

```bash
# 2. 验证无冲突：检查别名是否在结构体字段名中出现
# 例如：`BOSS_DATA` 可能在 `BossData` 字段的注释中（"BOSS_DATA: 構造体"）
# 确认无问题后批量替换

# 3. 用 sed 替换（建议每个别名独立 commit）
find GIAN07 -name "*.cpp" -o -name "*.h" | xargs sed -i 's/\bBOSS_DATA\b/BossData/g'
```

### 同步删除 `using` 行

替换完成后，从对应头文件中删除：
```cpp
using BOSS_DATA = BossData;  // 删除
```

### 验证

```bash
# 构建
./build_windows.bat

# 二次确认
rg "\bBOSS_DATA\b" GIAN07/   # 应为 0 结果
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 误替换字段名 | 编译失败 | 替换前 grep 二次确认 + 单个别名独立 commit |
| 头文件循环 | 编译失败 | 检查所有 include |
| 头文件依赖的间接引用 | 编译失败 | 全项目 grep |

### 提交

- 1 个 PR
- 内部按别名拆分 commit（建议每个别名 1 个 commit，共 21 个 commit）

---

## PR 3.1.B：删除批次 B 的 5 个大影响别名（中等影响面）

### 别名清单

| 别名 | 替换为 | 位置 | 影响面 |
| --- | --- | --- | --- |
| `TAMA_CMD` | `BulletCommand` | `bullet/TAMA.h:149` | 高（数百处） |
| `TAMA_DATA` | `Bullet` | `bullet/TAMA.h:150` | 高（数百处） |
| `LASER_CMD` | `LaserCommand` | `bullet/LASER.h:52` | 中 |
| `LLASER_CMD` | `LongLaserCommand` | `bullet/LLASER.h:48` | 中 |
| `LLASER_DATA` | `LongLaserData` | `bullet/LLASER.h:75` | 中 |
| `ENEMY_DATA` | `EnemyData` | `enemy/ENEMY.h:101` | 高 |
| `ITEM_DATA` | `ItemData` | `player/ITEM.h:35` | 中 |

### 子 PR 拆分

#### 子 PR 3.1.B.1：`TAMA_*` → `Bullet*`

**风险**：最高（数百处调用）。

**子操作**：
1. 全部 `TAMA_DATA` → `Bullet`。
2. 全部 `TAMA_CMD` → `BulletCommand`。
3. 删除 `TAMA.h:149-150` 的 `using` 行。
4. 检查头文件中的 `TAMA_xxx` 常量（`TAMA_MAX`、`TAMA_EVADE` 等）—— 这些是 `inline constexpr auto`，**不**是 `using` 别名，**保持不变**。

**注意**：`BulletManager` 类名 / `Bullet` 类型名与 `bullet/` 目录、`bullet_manager.h` 子目录命名一致，无冲突。

#### 子 PR 3.1.B.2：`ENEMY_DATA` → `EnemyData`

**风险**：高。

**子操作**：
1. 全部 `ENEMY_DATA` → `EnemyData`。
2. 删除 `ENEMY.h:101` 的 `using` 行。
3. `BossData` 中 `Edat` 字段类型从 `ENEMY_DATA` 改为 `EnemyData`。
4. `LLASER_DATA` 中的 `e` 字段从 `ENEMY_DATA*` 改为 `EnemyData*`。

#### 子 PR 3.1.B.3：`LASER_CMD` / `LLASER_*` / `ITEM_DATA`

**风险**：中。

**子操作**：
1. `LASER_CMD` → `LaserCommand`。
2. `LLASER_CMD` → `LongLaserCommand`。
3. `LLASER_DATA` → `LongLaserData`。
4. `ITEM_DATA` → `ItemData`。
5. 删除 `LASER.h:52`、`LLASER.h:48,75`、`ITEM.h:35` 的 `using` 行。

### 验证

每个子 PR 后：
```bash
./build_windows.bat
rg "\bENEMY_DATA\b" GIAN07/   # 应为 0 结果
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `TAMA_DATA` 误替换为 `Bullet` 时与 `BulletManager` 方法参数冲突 | 编译错误 | audit 每个方法签名 |
| `ENEMY_DATA` 在注释或字符串字面值中出现 | 误替换 | 用 `\b` 单词边界 sed |
| `Bullet` 类型与现有变量名冲突（如 `int bullet;`） | 编译错误 | grep `bullet` 局部变量名 |

---

## PR 3.2：`GameInit` 函数指针 → `std::function`

### 改动

#### `gameflow/GAMEMAIN.h:28`

旧：
```cpp
bool GameInit(void (*NextProc)(bool &quit)); // 游戏的初始化をする
```

新：
```cpp
[[nodiscard]] bool GameInit(std::function<void(bool &)> next_proc); // 游戏的初始化をする
```

#### `gameflow/GAMEMAIN.cpp:611-616`

旧：
```cpp
GameFlow.game_main = NextProc;
// Map known proc pointers to their states for GameMainIs-equiv checks
if (NextProc == GameProc)           GameFlow.current_state = GameState::Game;
else if (NextProc == DemoProc)      GameFlow.current_state = GameState::Demo;
else if (NextProc == ReplayProcAll) GameFlow.current_state = GameState::ReplayAll;
else                                GameFlow.current_state = GameState::External;
return true;
```

**保留 `current_state` 字段**（你的决策）。重构方式：

新：
```cpp
GameFlow.game_main = std::move(next_proc);
return true;
```

调用方负责在传入前设置 `current_state`：
```cpp
// 调用示例
GameFlow.current_state = GameState::Game;
GameInit([](bool &q) { GameProc(q); });
```

或保留 `current_state` 设置逻辑，**但用函数指针比较改为 lambda 类型比较**。`std::function` 不支持 `==` 运算符（除 `std::function` 内部为空比较）。所以推荐：调用方直接设置 `current_state`。

### 同步检查

- `gameflow/GAMEMAIN.cpp:649`（`GameReplayInitAll`）调用 `GameInit(ReplayProcAll)`。
- `gameflow/GAMEMAIN.cpp:774`（`DemoInit`）调用 `GameInit(DemoProc)`。
- `gameflow/GAMEMAIN.cpp:611`（`GameInit` 内）调用自身。

**所有调用方改为 lambda**：
```cpp
// 旧
GameInit(ReplayProcAll);

// 新
GameFlow.current_state = GameState::ReplayAll;
GameInit([](bool &q) { ReplayProcAll(q); });
```

或者**重载 `GameInit` 接受 `GameState`**：
```cpp
[[nodiscard]] bool GameInit(std::function<void(bool &)> next_proc, GameState state = GameState::External);
```

**决策**：保留你的选择（不改 `GameInit` 签名加 `state`），调用方负责设置。

### 验证

```bash
./build_windows.bat
# 启动游戏 → 进入标题 → 选择关卡 → 演示模式 → 重放模式
# 验证所有状态切换正常
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `std::function` 性能开销 | 函数调用开销（不热路径） | `GameInit` 仅在状态切换时调用，可接受 |
| 指针比较失效 | if/else 分支失效 | 改用调用方设置 `current_state` |
| lambda 捕获问题 | 链接失败 | audit lambda 捕获 |

---

## PR 3.3：删除 `EclInterpreter` 包装类

### 步骤

#### 1. 调研使用点

```bash
rg -n "EclInterpreter" GIAN07/
```

**预期位置**：
- `enemy/ecl_interpreter.h`（定义）
- `enemy/ecl_interpreter.cpp`（实现）
- 可能的外部使用点

#### 2. 重命名 `EnemyManager::ParseECL` 等方法

**新方法名**（与 `EclInterpreter` 对齐）：
```cpp
// 旧 → 新
void ParseECL(EnemyData *e) → void Execute(EnemyData *e)
void CheckECLInterrupt(EnemyData *e) → void CheckInterrupts(EnemyData *e)
void InitECLInterrupt(EnemyData *e) → void InitInterrupts(EnemyData *e)
void ECL_LongJump(EnemyData *e, uint32_t EclID) → void LongJump(EnemyData *e, uint32_t ecl_id)
```

#### 3. 更新所有 `Enemies.ParseECL(&e)` 等调用

```bash
# 替换
rg "Enemies.ParseECL" GIAN07/ -l | xargs sed -i 's/Enemies\.ParseECL/Enemies.Execute/g'
rg "Enemies.CheckECLInterrupt" GIAN07/ -l | xargs sed -i 's/Enemies\.CheckECLInterrupt/Enemies.CheckInterrupts/g'
rg "Enemies.InitECLInterrupt" GIAN07/ -l | xargs sed -i 's/Enemies\.InitECLInterrupt/Enemies.InitInterrupts/g'
rg "Enemies.ECL_LongJump" GIAN07/ -l | xargs sed -i 's/Enemies\.ECL_LongJump/Enemies.LongJump/g'
```

#### 4. 替换 `EclInterpreter` 调用为直接调用

```cpp
// 旧
EclInterpreter interp(ecl_data);
interp.Execute(e);

// 新
Enemies.Execute(&e);
```

#### 5. 删除 `EclInterpreter` 文件

```bash
git rm enemy/ecl_interpreter.h enemy/ecl_interpreter.cpp
```

#### 6. 从 `CMakeLists.txt` 移除

```cmake
# 旧
enemy/ecl_interpreter.cpp

# 新
# (已删除)
```

#### 7. 更新所有 `#include "ecl_interpreter.h"`

```bash
rg "ecl_interpreter.h" GIAN07/ -l | xargs sed -i '/#include "ecl_interpreter.h"/d'
```

### 验证

```bash
./build_windows.bat
rg "EclInterpreter" GIAN07/   # 应为 0 结果
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `EclInterpreter` 内部状态（`m_ecl_data`）丢失 | ECL 解释失败 | `EnemyManager::Execute` 直接使用 `Enemies.ecl_head` |
| 重命名 `ParseECL` 等方法破坏其他调用 | 编译失败 | 全项目 grep 后批量替换 |
| `EclInterpreter` 持有独立 `ecl_data` 但全局 `Enemies.ecl_head` 也存在 | 内存不一致 | 决策：使用全局 `ecl_head`（已存在） |

---

## PR 3.4：`Indsort` → `std::ranges::stable_partition`（保留 `indices[]` 间接访问）

### 设计决策

**保留 `indices[]` 间接访问语义**（你的选择）。仅替换 swap 算法。

### 新实现

`core/entity.h:14-45` 改为：

```cpp
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

template <class T, std::size_t N, typename ShouldDelete>
void Indsort(std::array<std::uint16_t, N> &indices, std::uint16_t &count,
             const std::array<T, N> &entities, ShouldDelete should_delete) noexcept {
  // stable_partition: 把 !should_delete 的索引排到前面
  // 保证相对顺序（与原 swap 算法行为不同但更稳定）
  const auto mid = std::stable_partition(
      indices.begin(), indices.begin() + count,
      [&entities, &should_delete](std::uint16_t i) {
        return !should_delete(entities[i]);
      });
  count = static_cast<std::uint16_t>(std::distance(indices.begin(), mid));
}
```

### 同步

#### `bullet/TAMA.h:171-176` 删除包装

```cpp
// 删除
template <std::size_t N>
void Indsort(std::array<std::uint16_t, N> &indices, std::uint16_t &count,
             const std::array<TAMA_DATA, N> &entities) {
  Indsort(indices, count, entities,
          [](const TAMA_DATA &t) { return (t.flag & TF_DELETE); });
}
```

#### `enemy/ENEMY.cpp:46-51` 删除包装

```cpp
// 删除
template <std::size_t N>
void Indsort(std::array<std::uint16_t, N> &indices, std::uint16_t &count,
             const std::array<EnemyData, N> &entities) {
  Indsort(indices, count, entities,
          [](const EnemyData &e) { return (e.flag & EF_DELETE); });
}
```

调用方改为直接使用核心 `Indsort`：
```cpp
// 旧
Indsort(Bullets.indices_small, this->count_small, this->bullets);

// 新
Indsort(Bullets.indices_small, this->count_small, this->bullets,
        [](const Bullet &t) { return (t.flag & TF_DELETE); });
```

### 行为差异分析

| 特性 | 旧（swap） | 新（stable_partition） |
| --- | --- | --- |
| 删除元素 | 移到末尾 | 移到末尾 |
| 相对顺序 | 可能变化（swap） | 保持 |
| 性能 | O(n) 单次遍历 + swap | O(n) 单次遍历 + 多次比较 |
| 异常安全 | 强（无 swap 抛异常） | 强（predicate noexcept 时） |

**结论**：行为差异在于相对顺序。如果游戏逻辑依赖 swap 后的特定顺序，需 audit。建议加单元测试覆盖。

### 单元测试（添加到 `tests/`）

```cpp
// tests/entity_test.cpp
#include "core/entity.h"
#include <gtest/gtest.h>

TEST(IndsortTest, KeepsAlive) {
  std::array<int, 5> entities = {1, 2, 3, 4, 5};
  std::array<std::uint16_t, 5> indices = {0, 1, 2, 3, 4};
  std::uint16_t count = 5;
  
  Indsort(indices, count, entities,
          [](const int &e) { return e % 2 == 0; });  // 删除偶数
  
  EXPECT_EQ(count, 3);  // 1, 3, 5
  EXPECT_EQ(entities[indices[0]], 1);
  EXPECT_EQ(entities[indices[1]], 3);
  EXPECT_EQ(entities[indices[2]], 5);
}

TEST(IndsortTest, StableOrder) {
  std::array<int, 5> entities = {1, 2, 3, 4, 5};
  std::array<std::uint16_t, 5> indices = {0, 1, 2, 3, 4};
  std::uint16_t count = 5;
  
  Indsort(indices, count, entities,
          [](const int &) { return false; });  // 不删除
  
  EXPECT_EQ(count, 5);
  // 验证顺序保持
  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(indices[i], i);
  }
}
```

### 验证

```bash
./build_windows.bat
# 0 错误

# 单元测试
ctest --test-dir build

# 游戏冒烟：跑一关，视觉无差异
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 顺序变化导致视觉异常 | 玩家看到不同行为 | 单元测试 + 冒烟 |
| 性能下降（多次比较） | 帧率下降 | 性能 profiling（不预期） |
| predicate 内部异常 | 异常路径 | 文档化 noexcept 要求 |
| 编译时模板展开失败 | 编译错误 | 用 `<algorithm>` 替代手写 swap |

---

## 阶段 3 总体验证

```bash
# 1. 构建
./build_windows.bat

# 2. 单元测试
ctest --test-dir build

# 3. 静态分析
clang-tidy --checks='-*,modernize-*' GIAN07/bullet/TAMA.cpp

# 4. 游戏冒烟
# 启动游戏，跑一关，验证视觉
```

## 风险登记表

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `using` 别名误替换 | 编译失败 | 单词边界 sed + 逐步 commit |
| `GameInit` 重构破坏状态机 | 游戏卡死 | 全路径状态切换测试 |
| `EclInterpreter` 删除遗漏引用 | 链接失败 | 全项目 grep 验证 0 引用 |
| `Indsort` 行为差异 | 视觉异常 | 单元测试 + 冒烟测试 |
| 多个 PR 合并冲突 | review 困难 | 严格按顺序合并 |

## 提交策略

| PR | commit 数 | 风险 |
| --- | --- | --- |
| 3.1.A | 21（每别名 1 个） | 低 |
| 3.1.B.1 | 1 | 中 |
| 3.1.B.2 | 1 | 中 |
| 3.1.B.3 | 1 | 低-中 |
| 3.2 | 1 | 低 |
| 3.3 | 1 | 中 |
| 3.4 | 1 | 中 |
| **总计** | **~27** | |
