///
/// GameplayHud - Player status and game telemetry presentation.
///

#pragma once

#include <cstdint>
#include <string_view>

#include "gameplay/game_rules.h"

struct GameplayHudModel {
  int64_t score = 0;
  int bombs = 0;
  int lives = 0;
  int credits = 0;
  int graze_count = 0;
  int graze_wait_time = 0;
  int miss_count = 0;
  int bomb_used = 0;
  int deathbomb_count = 0;
  int star_counter = 0;
  int star_threshold = 0;
  int rank = 0;
  std::string_view level_name;
  PracticeMode practice_mode = PracticeMode::Off;
};

class GameplayHud {
public:
  static void DrawTop(const GameplayHudModel &model);
  void DrawSidebars(const GameplayHudModel &model);

private:
  int64_t fps_sample_start_ = 0;
  int fps_ = 0;
  int frame_count_ = 0;
};
