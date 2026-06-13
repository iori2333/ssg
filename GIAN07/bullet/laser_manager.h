/*
 *   LaserManager — centralized laser system state
 */

#pragma once

#include "HOMINGL.h"
#include "LASER.h"
#include "LLASER.h"
#include <array>
#include <cstdint>

struct LaserManager {
  // --- 反射レーザー ---
  LaserCommand cmd;              // LaserCmd
  uint16_t count = 0;            // LaserNow

  // --- 長レーザー ---
  std::array<LongLaserData, LLASER_MAX> long_lasers; // LLaser[]
  LongLaserCommand long_cmd;     // LLaserCmd

  // --- ホーミングレーザー ---
  uint16_t homing_count = 0;     // HLaserNow
  HomingLaserInfo homing_cmd;    // HLaserCmd
  std::array<HomingLaserData, HLASER_MAX> homing_buf; // HLaserBuf[]
  HomingLaserData active;        // ActiveHL
  HomingLaserData free_list;     // FreeHL
};

extern LaserManager Lasers;
