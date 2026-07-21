# Manager 迁移方案

## 已完成的迁移

### 已删除的全局

| 全局 | 操作 |
|------|------|
| `Bullets` | 迁移到 `GameContext.bullets` |
| `Lasers` | 迁移到 `GameContext.lasers` |
| `Items` | 迁移到 `GameContext.items` |
| `Ranking` | 合并入 `GameManager`，迁移到 `GameContext.game` |
| `Games` | 合并入 `GameManager`，迁移到 `GameContext.game` |
| `kGfx*` 常量 | 全部合并为 `GameStage` enum |
| `MsgWin` | 删除（死代码） |
| `play_rank.h/cpp` | 删除（合并入 GameManager） |
| `rank_manager.h/cpp` | 删除（合并入 GameManager） |
| `pack_tool` | 从 CMakeLists.txt 中删除 |

### 当前架构

```
GameFlow (global)
  └── ctx: GameContext
       ├── BulletManager bullets
       ├── LaserManager lasers
       ├── ItemManager items
       └── GameManager game (count, stage, level, rank, AddRank, ResetRank)
```

### DI 模式

每个消费模块在 .h 中声明指针 + Bind() 重载：

```cpp
struct EnemyManager {
  BulletManager *bullets_ = nullptr;
  LaserManager *lasers_ = nullptr;
  ItemManager *items_ = nullptr;
  GameManager *game_ = nullptr;

  void Bind(BulletManager &bm) { bullets_ = &bm; }
  void Bind(LaserManager &lm) { lasers_ = &lm; }
  void Bind(ItemManager &im) { items_ = &im; }
  void Bind(GameManager &gm) { game_ = &gm; }
};
```

注入点统一在 `GameSTD_Init()` 中：

```cpp
Enemies.Bind(GameFlow.ctx.bullets);
Enemies.Bind(GameFlow.ctx.lasers);
Enemies.Bind(GameFlow.ctx.items);
Enemies.Bind(GameFlow.ctx.game);
// ...
GameFlow.ctx.bullets.Bind(GameFlow.ctx.items);
GameFlow.ctx.bullets.Bind(GameFlow.ctx.game);
GameFlow.ctx.lasers.Bind(GameFlow.ctx.game);
```

### GameStage

```cpp
enum class GameStage : uint8_t {
  NONE = 0,
  STAGE_1 = 1, ..., STAGE_6 = 6, CLEARED = 7,
  MUSIC_ROOM = 128, TITLE = 129, ..., ENDING = 135,
};
```

`gfx.LoadStage()`, `stage_mgr.LoadStageData()`, `Demos.LoadDemo()` 等函数签名直接接受 `GameStage`，调用方不需要 cast。

---

## 剩余待迁移的全局

### 第 2 层：战斗核心

| 全局 | 优先级 | 复杂度 | 说明 |
|------|--------|--------|------|
| `Enemies` | 高 | 大 | 自身是全局，但已有 DI 基础设施 |
| `Bosses` | 高 | 大 | 同上 |
| `Players` | 高 | 大 | 9 类未 DI 依赖需要处理 |

### 第 3 层：跨模块关切

| 全局 | 说明 |
|------|------|
| `Effects` | 15+ 种效果类型，9 个外部消费者 |
| `Scroller` | 已有 ranking_ DI |

### 第 5 层：非战斗子系统（仅由 gameflow 驱动，可最后处理）

| 全局 | 说明 |
|------|------|
| `ConfigDat` | 运行时常量 |
| `Demos` | 录制/回放 |
| `Scores` | 高分持久化，0 个外部消费者 |
| `Ending` | Credits 演出 |
| `UI` | UI 系统 |

### game/ 层基础设施

`Key_Data`、`Snd_SEPlay`、`GrpGeom` 等 — 跨平台抽象，暂不迁移。
