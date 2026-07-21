///
/// GameManager — Game session state and rank management
///

#pragma once

#include <cstdint>
#include <utility>

#include "config.h"
#include "level.h"

enum class GameStage : uint8_t {
  NONE = 0,

  STAGE_1 = 1,
  STAGE_2 = 2,
  STAGE_3 = 3,
  STAGE_4 = 4,
  STAGE_5 = 5,
  STAGE_6 = 6,

  CLEARED = 7,

  MUSIC_ROOM = 128,
  TITLE = 129,
  NAME_REGIST = 130,
  EXTRA = 131,
  EX_BOSS1 = 132,
  EX_BOSS2 = 133,
  S_PROJECT = 134,
  ENDING = 135,
};

constexpr auto operator<=>(GameStage a, GameStage b) {
  return std::to_underlying(a) <=> std::to_underlying(b);
}
constexpr bool operator==(GameStage a, GameStage b) { return a <=> b == 0; }

inline GameStage &operator++(GameStage &s) {
  s = static_cast<GameStage>(std::to_underlying(s) + 1);
  return s;
}

struct GameManager {
  uint32_t count = 0;
  GameStage stage = GameStage::STAGE_1;
  GameLevel level = GameLevel::NORMAL;
  PracticeMode practice_mode = PracticeMode::OFF;
  bool is_demoplay = false;
  int rank = 0;
  uint8_t extra_stg_flags = 0;
  bool bullet_gallery_active = false;

  const GameConfig *game_config_ = nullptr;

  [[nodiscard]] GameLevel EffectiveLevel() const;
  void AddRank(int n);
  void ResetRank();
};
