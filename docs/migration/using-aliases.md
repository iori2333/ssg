# using 别名清单

> Stage 0 盘点 | 日期: 2026-06-17 | 分支: refactor/modern-cpp

## 概述

共 38 个 `using` 声明（含模板别名和局部别名）。其中 28 个是"后向兼容"旧式大写别名，需要删除。

---

## 批次 A：低影响面别名（20 个）

这些别名使用范围有限，替换风险低。

| # | 别名 | 替换为 | 文件:行 |
| --- | --- | --- | --- |
| 1 | `FACE_DATA` | `FaceData` | `core/LOADER.h:80` |
| 2 | `ENDING_GRP` | `EndingGrp` | `core/LOADER.h:87` |
| 3 | `HLaserData` | `HomingLaserData` | `bullet/HOMINGL.h:40` |
| 4 | `HLaserInfo` | `HomingLaserInfo` | `bullet/HOMINGL.h:53` |
| 5 | `EXHITCHK` | `ExHitCheck` | `enemy/BOSS.h:18` |
| 6 | `INT_VECTOR` | `InterruptVector` | `enemy/ENEMY.h:102` |
| 7 | `MAID` | `Player` | `player/player_types.h:76` |
| 8 | `BOSS_DATA` | `BossData` | `enemy/BOSS.h:31` |
| 9 | `FRAGMENT_DATA` | `FragmentData` | `effect/FRAGMENT.h:32` |
| 10 | `DEMOPLAY_INFO` | `DemoPlayState` | `gameflow/DEMOPLAY.h:39` |
| 11 | `SCROLL_INFO` | `ScrollState` | `stage/SCROLL.h:90` |
| 12 | `SCL_INFO` | `SceneState` | `stage/SCROLL.h:91` |
| 13 | `CIRCLE_EFC_DATA` | `CircleEffectData` | `effect/EFFECT.h:61` |
| 14 | `SEFFECT_DATA` | `StringEffectData` | `effect/EFFECT.h:73` |
| 15 | `LOCKON_INFO` | `LockOnInfo` | `effect/EFFECT.h:82` |
| 16 | `SCREENEFC_INFO` | `ScreenEffectState` | `effect/EFFECT.h:88` |
| 17 | `BIT_PARAM` | `BitParam` | `enemy/EnemyExCtrl.h:62` |
| 18 | `BIT_DATA` | `BitData` | `enemy/EnemyExCtrl.h:89` |
| 19 | `NR_NAME_DATA` | `NrNameData` | `gameflow/SCORE.h:24` |
| 20 | `NR_SCORE_DATA` | `NrScoreData` | `gameflow/SCORE.h:33` |
| 21 | `NR_SCORE_STRING` | `NrScoreString` | `gameflow/SCORE.h:46` |

## 批次 B：大影响面别名（7 个）

这些别名遍布整个代码库，替换影响面大。

| # | 别名 | 替换为 | 文件:行 | 预估影响面 |
| --- | --- | --- | --- | --- |
| 22 | `TAMA_CMD` | `BulletCommand` | `bullet/TAMA.h:149` | 高（数百处） |
| 23 | `TAMA_DATA` | `Bullet` | `bullet/TAMA.h:150` | 高（数百处） |
| 24 | `LASER_CMD` | `LaserCommand` | `bullet/LASER.h:52` | 中 |
| 25 | `LLASER_CMD` | `LongLaserCommand` | `bullet/LLASER.h:48` | 中 |
| 26 | `LLASER_DATA` | `LongLaserData` | `bullet/LLASER.h:75` | 中 |
| 27 | `ENEMY_DATA` | `EnemyData` | `enemy/ENEMY.h:101` | 高 |
| 28 | `ITEM_DATA` | `ItemData` | `player/ITEM.h:35` | 中 |

---

## 非目标别名（保留）

以下 `using` 声明是模板别名、局部别名或合理命名，**不删除**：

| 别名 | 位置 | 原因 |
| --- | --- | --- |
| `BombEfcCtrl = BombEffectCtrl` | `effect/BOMBEFC.h:31` | snake_case 风格，仅缩短名称 |
| `OPTION<T> = CONFIG_OPTION_VALUE<T>` | `core/CONFIG.h:90` | 模板别名，合理简化 |
| `copy_opts = std::filesystem::copy_options` | `core/ENTRY.cpp:97` | 局部 using，非全局 |
| `FIT = GRAPHICS_FULLSCREEN_FIT` | `core/ENTRY.cpp:184` | 局部 using，非全局 |
| `_ = WINDOW_POINT` | `effect/GEOMETRY.h:51` | 内部 using |
| `fil_checksum_t = uint32_t` | `core/LZ_UTY.h:15` | 类型别名（非后向兼容） |
| `fil_size_t = uint32_t` | `core/LZ_UTY.h:16` | 类型别名 |
| `fil_no_t = uint32_t` | `core/LZ_UTY.h:17` | 类型别名 |
| `PlayRankInfo = PlayRankState` | `gameflow/PRankCtrl.h:15` | snake_case 别名 |
| `NR_SCORE_LIST` | `gameflow/score_manager.h:39` | 模板别名 |
| `NR_CONST_SCORE_LIST` | `gameflow/score_manager.h:40` | 模板别名 |
| `DATA_TYPE` | `enemy/EnemyExCtrl.cpp:82` | 局部 using |

---

## 验证命令

```bash
# 每个别名替换后确认 0 残留
rg "\bBOSS_DATA\b" GIAN07/    # 示例

# 全量确认
for a in FACE_DATA ENDING_GRP HLaserData HLaserInfo EXHITCHK INT_VECTOR \
         MAID BOSS_DATA FRAGMENT_DATA DEMOPLAY_INFO SCROLL_INFO SCL_INFO \
         CIRCLE_EFC_DATA SEFFECT_DATA LOCKON_INFO SCREENEFC_INFO BIT_PARAM \
         BIT_DATA NR_NAME_DATA NR_SCORE_DATA NR_SCORE_STRING \
         TAMA_CMD TAMA_DATA LASER_CMD LLASER_CMD LLASER_DATA ENEMY_DATA ITEM_DATA; do
  echo "=== $a ==="
  rg -c "\b$a\b" GIAN07/ 2>/dev/null || echo "0 (clean)"
done
```
