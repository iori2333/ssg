# Indsort 调用点清单

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 概述

`Indsort` 是一个模板函数，用于将标记为删除的实体移到数组末尾（类似 `std::stable_partition`）。

---

## 定义位置

| 文件:行 | 说明 |
| --- | --- |
| `core/entity.h:15` | **核心模板** `Indsort<T, N>(indices, count, entities, predicate)` |

### 包装函数

| 文件:行 | 说明 |
| --- | --- |
| `bullet/TAMA.h:172` | 特化：`Indsort(indices, count, entities)` — 用 `TF_DELETE` 谓词 |
| `enemy/ENEMY.cpp:47` | 特化：`Indsort(indices, count, entities)` — 用 `EF_DELETE` 谓词 |

---

## 调用点

### bullet/TAMA.cpp

| 行 | 代码 | 说明 |
| --- | --- | --- |
| 279 | `Indsort(Bullets.indices_small, this->count_small, this->bullets);` | 小子弹排序 |
| 307 | `Indsort(Bullets.indices_large, this->count_large, this->bullets);` | 大子弹排序 |
| 586 | `Indsort(Bullets.indices_small, this->count_small, this->bullets);` | 小子弹排序 |
| 600 | `Indsort(Bullets.indices_large, this->count_large, this->bullets);` | 大子弹排序 |
| 634 | `Indsort(Bullets.indices_small, this->count_small, this->bullets);` | 小子弹排序 |
| 654 | `Indsort(Bullets.indices_large, this->count_large, this->bullets);` | 大子弹排序 |

### enemy/ENEMY.cpp

| 行 | 代码 | 说明 |
| --- | --- | --- |
| 169 | `Indsort(this->indices, this->count, this->entities);` | 敌人排序 |
| 221 | `Indsort(this->indices, this->count, this->entities);` | 敌人排序 |

### bullet/LASER.cpp

| 行 | 代码 | 说明 |
| --- | --- | --- |
| 171 | `Indsort(Lasers.laser_indices, this->count, Lasers.lasers, ...);` | 激光排序 |

### player/ITEM.cpp

| 行 | 代码 | 说明 |
| --- | --- | --- |
| 118 | `Indsort(Items.indices, this->count, Items.entities, ...);` | 道具排序 |

### player/MAIDTAMA.cpp

| 行 | 代码 | 说明 |
| --- | --- | --- |
| 198 | `Indsort(Players.maid_tama_ind, this->maid_tama_now, Players.maid_tama, ...);` | 玩家子弹排序 |

---

## 共计

- **定义**：1 个核心 + 2 个包装 = 3 处
- **调用**：12 处
- **总计**：15 处

---

## Stage 3.4 迁移目标

将手写 swap 算法替换为 `std::ranges::stable_partition`，同时保留 `indices[]` 间接访问模式。

### 行为差异

| 特性 | 旧（swap） | 新（stable_partition） |
| --- | --- | --- |
| 删除元素处理 | 移到末尾 | 移到末尾 |
| 相对顺序 | 可能变化 | 保持 |
| 性能 | O(n) 单次遍历 | O(n) 单次遍历 |

---

## 验证

```bash
./build_windows.bat
ctest --test-dir build       # Stage 3.4 添加单元测试
```
