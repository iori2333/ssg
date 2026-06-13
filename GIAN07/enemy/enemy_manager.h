/*
 *   EnemyManager — centralized enemy system state
 */

#pragma once

#include "ENEMY.h"
#include "platform/buffer.h"
#include <array>
#include <cstdint>

struct EnemyManager {
  // --- 敵データ ---
  std::array<EnemyData, ENEMY_MAX> entities;   // Enemy[]
  std::array<uint16_t, ENEMY_MAX> indices;      // EnemyInd[]
  uint16_t count = 0;                            // EnemyNow

  // --- ECL/SCL データ ---
  BYTE_BUFFER_OWNED ecl_head;   // ECL_Head
  BYTE_BUFFER_OWNED scl_head;   // SCL_Head
  uint8_t* scl_now = nullptr;   // SCL_Now

  // --- アニメーション ---
  ANIME_DATA anime[ANIME_MAX];   // Anime[]

  // --- ホーミング ---
  int homing_x = 0;              // HomingX
  int homing_y = 0;              // HomingY
  int homing_flag = 0;           // HomingFlag

  // --- 特殊角度（ENEMY.cpp 内で使用）---
  uint8_t enemy_exdeg = 0;       // EnemyEXDEG
  uint8_t enemy_exdeg_d = 0;     // EnemyEXDEG_D
};

extern EnemyManager Enemies;
