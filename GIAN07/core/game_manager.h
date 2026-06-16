/*
 *   GameManager — centralized game flow state
 */

#pragma once

#include <cstdint>

struct GameManager {
  // --- ゲームフロー状態 ---
  uint32_t game_count = 0;
  uint8_t game_stage = 0;
  uint8_t game_level = 0;
  bool is_demoplay = false;
};

extern GameManager GameState;
