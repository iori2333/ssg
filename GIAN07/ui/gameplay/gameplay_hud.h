///
/// GameplayHud - Player status and game telemetry presentation.
///

#pragma once

#include <cstdint>
#include <string_view>

#include "core/config.h"

struct GameplayHudModel {
  int64_t score = 0;
  uint8_t bombs = 0;
  uint8_t lives = 0;
  uint8_t credits = 0;
  uint16_t graze_count = 0;
  uint16_t graze_wait_time = 0;
  uint16_t miss_count = 0;
  uint16_t bomb_used = 0;
  uint16_t deathbomb_count = 0;
  uint32_t star_counter = 0;
  uint32_t star_threshold = 0;
  int rank = 0;
  std::string_view level_name;
  PracticeMode practice_mode = PracticeMode::OFF;
};

class GameplayHud {
public:
  void DrawTop(const GameplayHudModel &model) const;
  void DrawSidebars(const GameplayHudModel &model);

private:
  uint32_t fps_sample_start_ = 0;
  uint32_t fps_ = 0;
  uint32_t frame_count_ = 0;
};
