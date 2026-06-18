///
/// GameManager - Centralized game flow state
///

#pragma once

#include <cstdint>

struct GameManager {
  // --- Game flow state ---
  uint32_t game_count = 0;
  uint8_t game_stage = 0;
  uint8_t game_level = 0;
  bool is_demoplay = false;
};

extern GameManager GameState;
