///
/// GameRules - shared game mode values
///

#pragma once

#include <cstdint>

enum class GameLevel : uint8_t {
  Easy = 0,
  Normal = 1,
  Hard = 2,
  Lunatic = 3,
  Extra = 4,
};

enum class PracticeMode : uint8_t {
  Off = 0,
  AutoBomb = 1,
  Invincible = 2,
};

enum class StageId : uint8_t {
  Stage1 = 0,
  Stage2,
  Stage3,
  Stage4,
  Stage5,
  Stage6,
  Extra,
};
