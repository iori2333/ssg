///
/// GameSession - game session state and rank management
///

#pragma once

#include <cstdint>
#include <string_view>

#include "game_rules.h"

struct GameSession {
  StageId stage = StageId::Stage1;
  GameLevel level = GameLevel::Normal;
  bool is_demoplay = false;
  int rank = 0;
  uint8_t extra_stg_flags = 0;

  [[nodiscard]] std::string_view DifficultyName() const;
  [[nodiscard]] GameLevel EffectiveLevel() const;
  void AdvanceStage();
  void UpdateRank(uint32_t stage_frame);
  void AddRank(int n);
  void ResetRank();
};
