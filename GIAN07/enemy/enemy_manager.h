/*
 *   EnemyManager — centralized enemy system state
 */

#pragma once

#include <array>
#include <cstdint>

#include "enemy/ENEMY.h"
#include "platform/buffer.h"

struct EnemyManager {
  // --- 敵データ ---
  std::array<EnemyData, ENEMY_MAX> entities; // Enemy[]
  std::array<uint16_t, ENEMY_MAX> indices;   // EnemyInd[]
  uint16_t count = 0;                        // EnemyNow

  // --- ECL/SCL データ ---
  BYTE_BUFFER_OWNED ecl_head; // ECL_Head
  BYTE_BUFFER_OWNED scl_head; // SCL_Head
  uint8_t *scl_now = nullptr; // SCL_Now

  // --- アニメーション ---
  ANIME_DATA anime[ANIME_MAX]; // Anime[]

  // --- ホーミング ---
  int homing_x = 0;    // HomingX
  int homing_y = 0;    // HomingY
  int homing_flag = 0; // HomingFlag
};

extern EnemyManager Enemies;
