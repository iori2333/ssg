# Stage 6 — 可选优化（3 个独立 PR）

## 目标
将前 5 个阶段未处理的"可选改进"项目化。

**风险等级**：低-中。

---

## PR 6.1：`BOSS_DATA::Hit` 改为 `std::unique_ptr<ExHitCheck>`

### 现状

`enemy/BOSS.h:23` 当前：
```cpp
struct BossData {
  ENEMY_DATA Edat;
  EXHITCHK *Hit;   // 特殊当たり判定(NULL なら使用しない)
  ...
};
```

`Hit` 是 raw pointer，`nullptr` 表示不使用。

### 目标

```cpp
struct BossData {
  EnemyData Edat;
  std::unique_ptr<ExHitCheck> hit;  // 空 = 未使用
  ...
};
```

### 改动列表

#### 头文件改动

`enemy/BOSS.h:23`：
```cpp
// 旧
EXHITCHK *Hit;

// 新
#include <memory>
std::unique_ptr<ExHitCheck> hit;
```

#### 实现文件改动

##### 初始化为 nullptr

`enemy/BOSS.cpp:68`：
```cpp
// 旧
it.Hit = nullptr;

// 新
it.hit.reset();
```

（`unique_ptr` 默认构造就是 `nullptr`，可省略）

##### 赋值

`enemy/BOSS.cpp` 搜索 `->Hit =`：
```bash
rg "\.Hit =|->Hit =" GIAN07/
```

每处替换：
```cpp
// 旧
b->Hit = some_pointer;

// 新
b->hit.reset(some_pointer);
```

##### 释放

`enemy/BOSS.cpp` 搜索 `delete ... Hit`：
```bash
rg "delete.*Hit" GIAN07/
```

每处替换：
```cpp
// 旧
delete b->Hit;
b->Hit = nullptr;

// 新
b->hit.reset();  // 自动 delete + 置 nullptr
```

##### 使用

`enemy/BOSS.cpp` 搜索 `if (b->Hit)`：
```cpp
// 旧
if (b->Hit) { ... }

// 新
if (b->hit) { ... }
```

或 `b->hit.get()` 取原始指针。

### 验证

```bash
./build_windows.bat
# 启动游戏，触发 boss 战，验证特殊当たり判定正常
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 漏改某处 `delete Hit` | 内存泄漏 | 编译 + 启动时 valgrind 验证 |
| 移动语义问题 | 编译错误 | `std::unique_ptr` 不可拷贝，需要时用 `std::move` |
| 第三方代码使用 `b->Hit` | 编译失败 | 全项目 grep |

---

## PR 6.2：静态局部状态 → 类成员

### 任务清单

#### 6.2.A：`NameRegistProc` 的 4 个 `static` → `NameRegistState`

**现状**（`gameflow/GAMEMAIN.cpp:303-306`）：
```cpp
void GameFlowManager::NameRegistProc(bool &) {
  // <- DemoInit() を修正するのだぞ
  PIXEL_LTRB src = {0, 0, 400, 64};
  int gx, gy, len;
  static int x, y;
  static int8_t key_time;
  static uint8_t count;
  static uint8_t time;
  ...
}
```

**目标**：抽 `NameRegistState` struct。

```cpp
// gameflow/name_regist_state.h
#pragma once

#include <cstdint>

struct NameRegistState {
  int x = 0;
  int y = 0;
  int8_t key_time = 0;
  uint8_t count = 0;
  uint8_t time = 0;
  
  void Reset();
  void Update(/* key input, etc. */);
};
```

**集成**：
- `GameFlowManager` 持有 `NameRegistState name_regist;`
- `NameRegistProc` 改为 `name_regist.Update(...)`。

#### 6.2.B：`Player::Draw` 的 `draw_flag` / `draw_flag2` → `Player` 成员

**现状**（`player/MAID.cpp:194-195`）：
```cpp
void Player::Draw() {
  static PIXEL_LTRB VivBit[4][2] = { ... };
  static uint8_t draw_flag = 0;
  static uint8_t draw_flag2 = 0;
  ...
}
```

**目标**：
```cpp
// player/player_types.h
struct Player {
  // ... 已有成员
  
  // 动画同步状态
  uint8_t draw_flag = 0;
  uint8_t draw_flag2 = 0;
  
  // ... 已有方法
};
```

**注意**：`VivBit[4][2]` 仍可保持 `static`（编译期常量），但应改 `static constexpr`。

#### 6.2.C：`Player::DrawWideBomb` 的 `static PIXEL_LTRB data[6]` → `static constexpr`

**现状**（`player/MAID.cpp:24-27`）：
```cpp
void Player::DrawWideBomb() {
  static PIXEL_LTRB data[6] = { ... };
  ...
}
```

**目标**：
```cpp
void Player::DrawWideBomb() {
  static constexpr PIXEL_LTRB data[6] = { ... };
  ...
}
```

#### 6.2.D：`GameFlowManager::ScoreNameProc` 的 `static const char *ExString[5]`

**现状**（`gameflow/GAMEMAIN.cpp:130-131`）：
```cpp
void GameFlowManager::ScoreNameProc(bool &) {
  static const char *ExString[5] = {"Easy", "Normal", "Hard", "Lunatic", "Extra"};
  ...
}
```

**目标**：提取为类成员或 `inline constexpr`：
```cpp
// gameflow/gameflow_manager.h
struct GameFlowManager {
  // ...
  static constexpr std::array<Narrow::string_view, 5> ExString = {
    "Easy", "Normal", "Hard", "Lunatic", "Extra"
  };
};
```

#### 6.2.E：审计其他静态局部变量

```bash
rg "static (int|uint|bool|char) " GIAN07/ --type cpp
```

逐个 review，决定是提升为成员还是 `static constexpr`。

### 验证

```bash
./build_windows.bat
# 启动游戏，进入名字注册流程，测试输入字母
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 状态提升为成员后序列化问题 | 保存/加载失败 | 当前无 save 状态，OK |
| 静态 → constexpr 编译期要求 | 编译错误 | 必须用 `constexpr` 表达式 |

---

## PR 6.3：加单元测试

### 任务清单

#### 6.3.A：测试基础设施

**位置**：`tests/CMakeLists.txt` + `tests/` 目录。

**工具**：
- gtest（已通过 git submodule 引入？）
- ctest（CMake 自带）

**`tests/CMakeLists.txt`**：
```cmake
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

add_executable(GIAN07_tests
  entity_test.cpp
  window_menu_test.cpp
  config_option_test.cpp
)

target_link_libraries(GIAN07_tests PRIVATE
  gtest_main
  # 项目库（待定义）
)

include(GoogleTest)
gtest_discover_tests(GIAN07_tests)
```

**集成到主 `CMakeLists.txt`**：
```cmake
# 在主 CMakeLists.txt 末尾
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
  enable_testing()
  add_subdirectory(tests)
endif()
```

#### 6.3.B：`core/entity.h::Indsort` 行为测试

**位置**：`tests/entity_test.cpp`。

**测试用例**（已在 stage 3.4 列出）：
- `Indsort_KeepsAlive`：删除偶数，保留奇数。
- `Indsort_StableOrder`：不删除时保持顺序。
- `Indsort_AllDeleted`：全部删除。
- `Indsort_NoneDeleted`：全部保留。

#### 6.3.C：`stage/WindowSys.h::WINDOW_MENU` 状态机测试

**位置**：`tests/window_menu_test.cpp`。

**测试用例**：
- `MenuInit_Default`：默认状态。
- `MenuOpen_SetSelect`：选择项设置。
- `MenuClose_ToParent`：子菜单关闭。
- `MenuMove_KeyUp`：上箭头处理。

#### 6.3.D：`core/CONFIG.h::CONFIG_OPTION_VALUE` 验证器测试

**位置**：`tests/config_option_test.cpp`。

**测试用例**：
- `OptionLoad_Valid`：值有效。
- `OptionLoad_InvalidReset`：值无效，重置默认。
- `OptionLoad_MissingDefaults`：值缺失，使用默认。
- `Validator_Below_Pass`：低于上限。
- `Validator_Below_Fail`：超过上限。
- `Validator_Mask_Pass`：掩码匹配。
- `Validator_Mask_Fail`：掩码不匹配。

#### 6.3.E：CI 集成

`.github/workflows/tests.yml`：
```yaml
name: Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main, develop]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Configure
        run: cmake -B build -DBUILD_TESTING=ON

      - name: Build
        run: cmake --build build

      - name: Test
        run: ctest --test-dir build --output-on-failure
```

### 验证

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### 风险与缓解

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 单元测试依赖未声明的内部状态 | 编译错误 | 测试文件仅 include 必要头文件 |
| 状态机测试不覆盖所有路径 | bug 漏检 | 持续补充测试 |
| gtest 子模块未引入 | 配置失败 | 检查 `libs/googletest` 是否存在 |

---

## 阶段 6 总体验证

```bash
# 1. 构建
./build_windows.bat

# 2. 单元测试
cmake -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure

# 3. 静态分析
clang-tidy GIAN07/enemy/boss.cpp -- -std=c++20
```

---

## 风险登记表

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| `unique_ptr` 移动语义问题 | 编译错误 | audit 所有 `BossData` 拷贝/移动 |
| 状态提升破坏存档/读档 | 数据丢失 | 当前无存档功能 |
| gtest 引入增加编译时间 | CI 慢 | 仅在 `BUILD_TESTING=ON` 时编译 |
| 测试覆盖率不足 | 漏检 | 持续补充测试用例 |

---

## 提交策略

| PR | commit 数 | 风险 |
| --- | --- | --- |
| 6.1 | 1 | 中 |
| 6.2 | 4-5（每个子任务 1 个） | 低 |
| 6.3 | 5+（基础设施 + 每个测试文件） | 低 |
| **总计** | **~10** | |

**每个 PR 独立**：
- 6.1：BossData::Hit 改为 unique_ptr
- 6.2.A：NameRegistState
- 6.2.B：Player::draw_flag 成员化
- 6.2.C：DrawWideBomb static constexpr
- 6.2.D：ScoreNameProc ExString 成员化
- 6.2.E：审计其他 static
- 6.3.A：测试基础设施
- 6.3.B：Indsort 测试
- 6.3.C：WINDOW_MENU 测试
- 6.3.D：CONFIG_OPTION_VALUE 测试
- 6.3.E：CI 集成

---

## 后续阶段衔接

完成阶段 6 后：
- 全部 6 个阶段完成
- 项目进入维护期
- 后续改进可通过新阶段（stage 7+）继续
