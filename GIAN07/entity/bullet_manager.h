/*
 *   BulletManager — centralized bullet system state
 */

#pragma once

#include <array>
#include <cstdint>
#include "entity/TAMA.h"

struct BulletManager {
  // --- 弾データ ---
  std::array<Bullet, TAMA_MAX> bullets;         // Tama[]
  BulletCommand command;                        // TamaCmd
  std::array<uint16_t, TAMA_MAX> indices_small; // Tama1Ind[]
  std::array<uint16_t, TAMA_MAX> indices_large; // Tama2Ind[]
  uint16_t count_small = 0;                     // Tama1Now
  uint16_t count_large = 0;                     // Tama2Now
  uint16_t max_small = 0;                       // Tama1Max
  uint16_t max_large = 0;                       // Tama2Max
  int speed = 0;                                // TamaSpeed
};

extern BulletManager Bullets;
