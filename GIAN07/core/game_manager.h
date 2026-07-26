///
/// GameManager — Game session state and rank management
///

#pragma once

#include <cstdint>
#include <utility>

#include "config.h"
#include "level.h"

enum class StageId : uint8_t {
  STAGE_1 = 0,
  STAGE_2,
  STAGE_3,
  STAGE_4,
  STAGE_5,
  STAGE_6,
  EXTRA,
};

constexpr auto operator<=>(StageId a, StageId b) {
  return std::to_underlying(a) <=> std::to_underlying(b);
}
constexpr bool operator==(StageId a, StageId b) { return a <=> b == 0; }

inline StageId &operator++(StageId &s) {
  s = static_cast<StageId>(std::to_underlying(s) + 1);
  return s;
}

struct GameManager {
  StageId stage = StageId::STAGE_1;
  GameLevel level = GameLevel::NORMAL;
  PracticeMode practice_mode = PracticeMode::OFF;
  bool is_demoplay = false;
  int rank = 0;
  uint8_t extra_stg_flags = 0;

  const GameConfig *game_config_ = nullptr;

  [[nodiscard]] std::string_view LevelName() const;
  [[nodiscard]] GameLevel EffectiveLevel() const;
  void Update(uint32_t stage_frame);
  void AddRank(int n);
  void ResetRank();
};
