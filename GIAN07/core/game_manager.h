///
/// GameManager - Centralized game flow state
///

#pragma once

#include <cstdint>

#include "level.h"

struct GameManager {
  // --- Game flow state ---
  uint32_t game_count = 0;
  uint8_t game_stage = 0;
  GameLevel game_level = GameLevel::NORMAL;
  bool is_demoplay = false;
};

extern GameManager GameState;
