# this-> 与 Bullets. 自引用统计

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 概述

**总计**: 855 处 `this->` 出现在 15 个 .cpp 文件中。

---

## this-> 按文件统计

| # | 文件 | 数量 | 优先级 |
| --- | --- | --- | --- |
| 1 | `bullet/TAMA.cpp` | 150 | 高 |
| 2 | `enemy/EnemyExCtrl.cpp` | 142 | 高 |
| 3 | `enemy/BOSS.cpp` | 114 | 高 |
| 4 | `gameflow/ENDING.cpp` | 114 | 高 |
| 5 | `gameflow/DEMOPLAY.cpp` | 95 | 中 |
| 6 | `bullet/LASER.cpp` | 65 | 中 |
| 7 | `enemy/ENEMY.cpp` | 47 | 中 |
| 8 | `gameflow/PRankCtrl.cpp` | 40 | 中 |
| 9 | `gameflow/SCORE.cpp` | 37 | 中 |
| 10 | `player/MAID.cpp` | 18 | 低 |
| 11 | `player/ITEM.cpp` | 10 | 低 |
| 12 | `effect/EFFECT3D.cpp` | 9 | 低 |
| 13 | `player/MAIDTAMA.cpp` | 8 | 低 |
| 14 | `bullet/HOMINGL.cpp` | 5 | 低 |
| 15 | `bullet/LLASER.cpp` | 1 | 低 |
| **总计** | | **855** | |

---

## Bullets. 自引用（bullet/TAMA.cpp）

在 `BulletManager` 方法体内，`Bullets.xxx` 应改为直接成员名（`xxx`）。

**出现行号**（约 34 处）：
53, 55, 61, 71, 75, 77, 127, 131, 133, 196, 205, 207, 255, 279, 283, 307, 325, 428, 549, 559, 576, 586, 590, 600, 618, 634, 638, 654, 666, 667, 671, 673, 768, 790

**示例**：
```cpp
// 旧：BulletManager 方法体内
Bullets.speed = v;             // → speed = v;
indp = &Bullets.indices_small; // → indp = &indices_small;
```

---

## 清理原则

- **删除**：简单成员读写 `this->xxx` → `xxx`
- **保留**：参数与成员同名（消歧义）
- **保留**：限定符调用（如 `BossManager::STDMove`）
- **保留**：模板依赖基类成员访问

## 验证

```bash
./build_windows.bat   # 每个子 PR 后验证编译
```
