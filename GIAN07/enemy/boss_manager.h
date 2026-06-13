/*
 *   BossManager — centralized boss system state
 */

#pragma once

#include "BOSS.h"
#include <array>
#include <cstdint>

struct BossManager {
  std::array<BOSS_DATA, BOSS_MAX> bosses;     // Boss[]
  uint16_t count = 0;                          // BossNow
  BOSSHPG_INFO hpg;                            // BossHPG
};

extern BossManager Bosses;
// 後方互換用参照は BOSS.h で宣言
