///
/// GameSession - game session state and fixed difficulty rank
///

#pragma once

#include "game_rules.h"

struct GameSession {
  StageId stage = StageId::Stage1;
  GameLevel level = GameLevel::Normal;
  bool is_demoplay = false;

  [[nodiscard]] int Rank() const;
  void AdvanceStage();
};
