# Stage 4 — 8.3 文件名重命名（按子目录分批，7 个 PR）

## 目标
将 8.3 时代遗留的大写文件名（`GIAN.cpp`、`TAMA.cpp` 等）改为 snake_case 风格。

**风险等级**：中-高（影响 #include 路径、CMake、IDE、Git 历史）。

**执行原则**：按子目录分批（你的决策），每批独立编译验证。

---

## 总体重命名映射

| 旧名 | 新名 | 子目录 |
| --- | --- | --- |
| `core/GIAN.cpp` | `core/gian.cpp` | core/ |
| `core/GIAN.h` | `core/gian.h` | core/ |
| `core/GIAN07.rc` | `core/gian07.rc` | core/ |
| `core/ENTRY.cpp` | `core/entry.cpp` | core/ |
| `core/ENTRY.h` | `core/entry.h` | core/ |
| `core/EXDEF.h` | `core/point.h` | core/ |
| `core/LEVEL.h` | `core/level.h` | core/ |
| `core/LOADER.cpp` | `core/loader.cpp` | core/ |
| `core/LOADER.h` | `core/loader.h` | core/ |
| `core/CONFIG.cpp` | `core/config.cpp` | core/ |
| `core/CONFIG.h` | `core/config.h` | core/ |
| `core/LZ_UTY.cpp` | `core/lz_uty.cpp` | core/ |
| `core/LZ_UTY.h` | `core/lz_uty.h` | core/ |
| `bullet/TAMA.cpp` | `bullet/bullet.cpp` | bullet/ |
| `bullet/TAMA.h` | `bullet/bullet.h` | bullet/ |
| `bullet/LASER.cpp` | `bullet/laser.cpp` | bullet/ |
| `bullet/LASER.h` | `bullet/laser.h` | bullet/ |
| `bullet/LLASER.cpp` | `bullet/long_laser.cpp` | bullet/ |
| `bullet/LLASER.h` | `bullet/long_laser.h` | bullet/ |
| `bullet/HOMINGL.cpp` | `bullet/homing_laser.cpp` | bullet/ |
| `bullet/HOMINGL.h` | `bullet/homing_laser.h` | bullet/ |
| `enemy/ENEMY.cpp` | `enemy/enemy.cpp` | enemy/ |
| `enemy/ENEMY.h` | `enemy/enemy.h` | enemy/ |
| `enemy/BOSS.cpp` | `enemy/boss.cpp` | enemy/ |
| `enemy/BOSS.h` | `enemy/boss.h` | enemy/ |
| `effect/EFFECT.cpp` | `effect/effect.cpp` | effect/ |
| `effect/EFFECT.h` | `effect/effect.h` | effect/ |
| `effect/EFFECT3D.cpp` | `effect/effect3d.cpp` | effect/ |
| `effect/EFFECT3D.h` | `effect/effect3d.h` | effect/ |
| `player/MAID.cpp` | `player/player.cpp` | player/ |
| `player/MAID.h` | `player/player.h` | player/ |
| `player/MAIDTAMA.cpp` | `player/player_shot.cpp` | player/ |
| `player/MAIDTAMA.h` | `player/player_shot.h` | player/ |
| `stage/SCROLL.cpp` | `stage/scroll.cpp` | stage/ |
| `stage/SCROLL.h` | `stage/scroll.h` | stage/ |
| `stage/SCL.h` | `stage/scene.h` | stage/ |
| `stage/WINDOWSYS.cpp` | `stage/window_sys.cpp` | stage/ |
| `stage/WINDOWSYS.h` | `stage/window_sys.h` | stage/ |
| `stage/WINDOWCTRL.cpp` | `stage/window_ctrl.cpp` | stage/ |
| `stage/WINDOWCTRL.h` | `stage/window_ctrl.h` | stage/ |
| `gameflow/GAMEMAIN.cpp` | `gameflow/game_main.cpp` | gameflow/ |
| `gameflow/GAMEMAIN.h` | `gameflow/game_main.h` | gameflow/ |
| `gameflow/ENDING.cpp` | `gameflow/ending.cpp` | gameflow/ |
| `gameflow/ENDING.h` | `gameflow/ending.h` | gameflow/ |
| `gameflow/DEMOPLAY.cpp` | `gameflow/demo_play.cpp` | gameflow/ |
| `gameflow/DEMOPLAY.h` | `gameflow/demo_play.h` | gameflow/ |
| `gameflow/PRankCtrl.cpp` | `gameflow/play_rank.cpp` | gameflow/ |
| `gameflow/PRankCtrl.h` | `gameflow/play_rank.h` | gameflow/ |
| `gameflow/SCORE.cpp` | `gameflow/score.cpp` | gameflow/ |
| `gameflow/SCORE.h` | `gameflow/score.h` | gameflow/ |

**总计**：55 个文件（27 对 `.h/.cpp` + 1 个 `.rc`）。

---

## 通用操作流程（每个 PR）

### 1. Git 重命名

```bash
# 1.1 单个文件重命名（保留 git 历史）
git mv OLD_NAME NEW_NAME

# 1.2 批量重命名（脚本）
for f in GIAN.cpp GIAN.h ENTRY.cpp ENTRY.h; do
  lower=$(echo "$f" | tr 'A-Z' 'a-z')
  git mv "core/$f" "core/$lower"
done
```

### 2. 替换 `#include` 路径

```bash
# 2.1 找出所有引用
rg -l '#include "GIAN.h"' GIAN07/

# 2.2 批量替换
find GIAN07 -name "*.cpp" -o -name "*.h" | xargs sed -i 's|#include "GIAN.h"|#include "gian.h"|g'

# 2.3 子目录 include（如果有）
rg -l '#include "../GIAN.h"' GIAN07/
# 替换为相对路径 #include "../gian.h"
```

### 3. 更新 `CMakeLists.txt`

```cmake
# 旧
set(GIAN07_SOURCES
    GIAN07/GIAN.cpp
    GIAN07/GIAN.h
    ...
)

# 新
set(GIAN07_SOURCES
    GIAN07/gian.cpp
    GIAN07/gian.h
    ...
)
```

### 4. 更新 `.rc` 文件（如有）

```rc
// 旧
#include "GIAN07.h"

// 新
#include "gian.h"
```

### 5. 验证

```bash
# 构建
./build_windows.bat

# 在 WSL 或 Linux 容器内（如果有 CI）
./build_linux.sh

# 启动游戏，验证启动
```

### 6. 提交

```bash
git add -A
git commit -m "rename core/ files to snake_case (Stage 4.1)"
```

---

## PR 4.1：core/ 重命名

### 涉及文件（14 个）

```
core/GIAN.cpp    → core/gian.cpp
core/GIAN.h      → core/gian.h
core/GIAN07.rc   → core/gian07.rc
core/ENTRY.cpp   → core/entry.cpp
core/ENTRY.h     → core/entry.h
core/EXDEF.h     → core/point.h
core/LEVEL.h     → core/level.h
core/LOADER.cpp  → core/loader.cpp
core/LOADER.h    → core/loader.h
core/CONFIG.cpp  → core/config.cpp
core/CONFIG.h    → core/config.h
core/LZ_UTY.cpp  → core/lz_uty.cpp
core/LZ_UTY.h    → core/lz_uty.h
```

### 风险

- `EXDEF.h → point.h` 需要先 grep 确认 `point.h` 不与 `game/coords.h` 冲突。
- `point.h` 中的 `DegPoint` 与 `game/coords.h` 的 `WORLD_POINT`、`PIXEL_POINT` 同名空间，需要 audit。

### 验证

```bash
# grep 确认无冲突
rg "point.h" GIAN07/ --files-with-matches
# 应只显示 core/point.h（新）和 game/coords.h（已有）

# 构建
./build_windows.bat
```

---

## PR 4.2：bullet/ 重命名

### 涉及文件（8 个）

```
bullet/TAMA.cpp    → bullet/bullet.cpp
bullet/TAMA.h      → bullet/bullet.h
bullet/LASER.cpp   → bullet/laser.cpp
bullet/LASER.h     → bullet/laser.h
bullet/LLASER.cpp  → bullet/long_laser.cpp
bullet/LLASER.h    → bullet/long_laser.h
bullet/HOMINGL.cpp → bullet/homing_laser.cpp
bullet/HOMINGL.h   → bullet/homing_laser.h
```

### 风险

- `bullet/bullet.cpp` 与 `bullet/bullet_manager.cpp` 同目录，新名直接使用 `bullet` 单词，可能与 `Bullet` 类型混淆。考虑改为 `bullet_main.cpp` 或 `bullet_array.cpp`。
  - **建议**：仍用 `bullet.cpp`（与 `Bullet` 类型一致，且子目录 `bullet/` 与 `bullet_manager.h` 命名风格统一）。
- `bullet/long_laser.cpp` 与 `bullet/laser.cpp` 同目录，命名约定清晰。

### 验证

```bash
./build_windows.bat
```

---

## PR 4.3：enemy/ 重命名

### 涉及文件（4 个）

```
enemy/ENEMY.cpp → enemy/enemy.cpp
enemy/ENEMY.h   → enemy/enemy.h
enemy/BOSS.cpp  → enemy/boss.cpp
enemy/BOSS.h    → enemy/boss.h
```

### 风险

- `enemy/boss.cpp` 与 `enemy/boss_manager.cpp` 同目录，命名约定清晰。
- `enemy/EnemyExCtrl.cpp/h`（已 snake_case）保持。

### 验证

```bash
./build_windows.bat
```

---

## PR 4.4：effect/ 重命名

### 涉及文件（4 个）

```
effect/EFFECT.cpp   → effect/effect.cpp
effect/EFFECT.h     → effect/effect.h
effect/EFFECT3D.cpp → effect/effect3d.cpp
effect/EFFECT3D.h   → effect/effect3d.h
```

### 风险

- `effect/effect.cpp` 与 `effect/effect_manager.cpp` 同目录，命名约定清晰。
- `effect3d` 在很多平台上是常用子模块名，无冲突。

### 验证

```bash
./build_windows.bat
```

---

## PR 4.5：player/ 重命名

### 涉及文件（4 个）

```
player/MAID.cpp     → player/player.cpp
player/MAID.h       → player/player.h
player/MAIDTAMA.cpp → player/player_shot.cpp
player/MAIDTAMA.h   → player/player_shot.h
```

### 风险

- `player/player.cpp` 与 `player/player_manager.cpp`、`player/player_types.h` 同目录。命名空间紧张。
- 建议**重命名** `player_manager.cpp` → `player_system.cpp` 或类似以避免混淆？**不**，本 PR 不动其他文件。
- `player/player.cpp` 含 `Player` 类方法实现；`player/player_types.h` 含 `Player` 类定义。两者分离清晰。

### 验证

```bash
./build_windows.bat
```

---

## PR 4.6：stage/ 重命名

### 涉及文件（7 个）

```
stage/SCROLL.cpp      → stage/scroll.cpp
stage/SCROLL.h        → stage/scroll.h
stage/SCL.h           → stage/scene.h
stage/WINDOWSYS.cpp   → stage/window_sys.cpp
stage/WINDOWSYS.h     → stage/window_sys.h
stage/WINDOWCTRL.cpp  → stage/window_ctrl.cpp
stage/WINDOWCTRL.h    → stage/window_ctrl.h
```

### 风险

- `stage/scene.h`（旧 `SCL.h`）与 `gameflow/scene` 等不冲突。但 `SceneState` 已存在于 `SCROLL.h` 中，新 `scene.h` 应仅含 SCL（场景）常量定义。
- `stage/window_sys.cpp` 与 `stage/window_ctrl.cpp` 命名约定清晰。

### 验证

```bash
./build_windows.bat
```

---

## PR 4.7：gameflow/ 重命名

### 涉及文件（10 个）

```
gameflow/GAMEMAIN.cpp → gameflow/game_main.cpp
gameflow/GAMEMAIN.h   → gameflow/game_main.h
gameflow/ENDING.cpp   → gameflow/ending.cpp
gameflow/ENDING.h     → gameflow/ending.h
gameflow/DEMOPLAY.cpp → gameflow/demo_play.cpp
gameflow/DEMOPLAY.h   → gameflow/demo_play.h
gameflow/PRankCtrl.cpp → gameflow/play_rank.cpp
gameflow/PRankCtrl.h  → gameflow/play_rank.h
gameflow/SCORE.cpp    → gameflow/score.cpp
gameflow/SCORE.h      → gameflow/score.h
```

### 风险

- `gameflow/score.cpp` 与 `gameflow/score_manager.cpp` 同目录。命名约定清晰。
- `gameflow/demo_play.cpp` 与 `gameflow/demo_manager.cpp` 同目录。命名约定清晰。
- `gameflow/game_main.cpp` 与 `gameflow/gameflow_manager.cpp` 同目录。命名约定清晰。

### 验证

```bash
./build_windows.bat
# 启动游戏，跑完整流程
```

---

## 跨子目录依赖关系

### include 引用图

```
core/gian.h        ← (几乎所有文件)
core/entry.h       ← (初始化相关)
core/level.h       ← (难度常量)
core/loader.h      ← (资源加载)
core/config.h      ← (配置)
core/lz_uty.h      ← (压缩)
core/point.h       ← (几何点)

bullet/bullet.h        ← enemy/enemy.h, player/player.h
bullet/laser.h         ← enemy/enemy.h
bullet/long_laser.h    ← enemy/enemy.h
bullet/homing_laser.h  ← enemy/enemy.h

enemy/enemy.h      ← bullet/bullet.h, bullet/laser.h
enemy/boss.h       ← enemy/enemy.h
enemy/ecl_interpreter.h ← (删除后无引用)

effect/effect.h    ← enemy/enemy.h
effect/effect3d.h  ← effect/effect.h
effect/fragment.h  ← effect/effect.h
effect/bombefc.h   ← effect/effect.h

player/player.h        ← core/gian.h
player/player_shot.h   ← bullet/bullet.h
player/item.h          ← core/gian.h
player/player_types.h  ← core/gian.h

stage/scroll.h     ← core/gian.h
stage/scene.h      ← (SCL 定义)
stage/window_sys.h ← (UI)
stage/window_ctrl.h ← stage/window_sys.h

gameflow/game_main.h    ← (几乎所有)
gameflow/ending.h       ← gameflow/game_main.h
gameflow/demo_play.h    ← gameflow/game_main.h
gameflow/play_rank.h    ← gameflow/game_main.h
gameflow/score.h        ← gameflow/game_main.h
```

**结论**：每个子目录重命名 PR 内部一致即可；跨子目录依赖通过 #include 路径更新。

---

## 阶段 4 总体验证

### 1. 构建

```bash
./build_windows.bat
# 0 错误 0 警告
```

### 2. Linux 构建（如果适用）

```bash
./build_linux.sh
# 大小写敏感验证
```

### 3. IDE 索引重建

- CLion：`File → Invalidate Caches / Restart`
- VS：`Close Solution → Reopen`
- VSCode：`Developer: Reload Window`

### 4. 启动游戏

```bash
build/bin/GIAN07.exe
```

- 进入标题画面
- 选择关卡
- 玩一关
- 触发所有功能（boss、Ending、Replay、Demo）
- 视觉、操控、声音无异常

### 5. Git 历史验证

```bash
# 验证 git rename 检测到
git log --follow core/gian.cpp
# 应显示 GIAN.cpp 的历史

# 统计改名后保留的行历史
git diff --stat --find-renames HEAD~7 HEAD
```

---

## 风险登记表

| 风险 | 影响 | 缓解 |
| --- | --- | --- |
| 大小写冲突（Windows 不敏感 vs Linux 敏感） | Linux CI 失败 | 每个 PR 后在 WSL/Linux 验证 |
| 旧名与新名同时存在（暂态） | 编译错误 | 一次性 `git mv` + 立即替换 include |
| `EXDEF.h → point.h` 与 `game/coords.h` 的 `POINT_*` 类型冲突 | 编译错误 | audit `point.h` 中的 `DegPoint` 与 `PIXEL_POINT` |
| 工具链缓存（CMake、IDE） | 重新配置耗时 | `cmake --build . --target clean` + 删除 `build/` |
| `bullet/bullet.cpp` 文件名歧义 | 维护困惑 | 通过目录区分（`bullet/` 下的 `bullet.cpp` 是合理的） |
| 7 个 PR 合并冲突 | review 困难 | 严格按顺序合并（4.1 → 4.7） |

---

## 提交策略

| PR | commit 数 | 风险 |
| --- | --- | --- |
| 4.1 | 1（一次 git mv 全部） | 中 |
| 4.2 | 1 | 中 |
| 4.3 | 1 | 中 |
| 4.4 | 1 | 中 |
| 4.5 | 1 | 中 |
| 4.6 | 1 | 中 |
| 4.7 | 1 | 中 |
| **总计** | **7** | |

**每个 PR 内部**：
- 1 个 commit 包含所有 git mv
- 1 个 commit 包含所有 include 替换
- 1 个 commit 包含所有 CMakeLists.txt 更新

或者 1 个大 commit（推荐单 commit 便于 review）。

---

## 后续阶段衔接

完成阶段 4 后：
- 阶段 5（静态分析）开始
- 阶段 6（可选优化）开始
- 不再有"大写文件名"遗留
