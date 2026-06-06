/*
 *   GameManager — centralized game flow state
 */

#pragma once

#include <cstdint>

struct GameManager {
  static GameManager &Instance();

  // --- ゲームフロー状態 ---
  uint32_t game_count = 0;
  uint8_t game_stage = 0;
  uint8_t game_level = 0;
  bool is_demoplay = false;

private:
  GameManager() = default;
};
