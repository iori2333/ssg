# // was xxx 迁移注释清单

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 概述

**总计**: 125 处 `// was xxx` 注释，分布在 10 个文件中。

注意：计划预估 200+ 处，实际 125 处。部分文件（`effect/effect_manager.h`、`gameflow/gameflow_manager.h`、`gameflow/score_manager.h`）可能已在之前的重构中清理。

---

## 按文件统计

| # | 文件 | 数量 | 备注 |
| --- | --- | --- | --- |
| 1 | `bullet/laser_manager.h` | 36 | 最多 |
| 2 | `bullet/bullet_manager.h` | 23 | |
| 3 | `enemy/boss_manager.h` | 19 | |
| 4 | `enemy/enemy_manager.h` | 16 | |
| 5 | `gameflow/demo_manager.h` | 11 | |
| 6 | `stage/scroll_manager.h` | 5 | |
| 7 | `player/player_manager.h` | 5 | |
| 8 | `player/item_manager.h` | 4 | |
| 9 | `gameflow/ending_manager.h` | 4 | 计划未列出，新增发现 |
| 10 | `gameflow/rank_manager.h` | 2 | |
| **总计** | | **125** | |

---

## 示例内容

典型的 `// was xxx` 注释是之前 C 风格全局函数重命名为 C++ 方法后的迁移痕迹：

```cpp
// bullet/bullet_manager.h
void Move();                   // was TamaMove
void Draw();                   // was TamaDraw
void Init(uint8_t type);       // was TamaInit

// enemy/boss_manager.h
void Init();                   // was BossDataInit
void Move();                   // was BossMove
void Draw();                   // was BossDraw
bool ApplyDamage(...);          // was BossDamageApply

// player/player_manager.h
void SetMaidShot();            // was MaidTamaSet
void MoveMaidShot();           // was MaidTamaMove
```

---

## 操作计划（Stage 1.2）

1. 逐个文件手动 review，确认每行 `// was xxx` 仅是迁移痕迹
2. 批量删除，保留设计意图注释（如 `// 弾の最大発生数`）
3. 不删除 `// ==` 或 `// ===` 分隔注释

## 验证

```bash
rg "// was " GIAN07/   # 应为 0 结果
./build_windows.bat    # 编译通过
```
