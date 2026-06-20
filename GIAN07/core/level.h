///
/// Level - Difficulty & option settings
///

#pragma once

#include <cstdint>

enum class GameLevel : uint8_t {
  EASY = 0,
  NORMAL = 1,
  HARD = 2,
  LUNATIC = 3,
  EXTRA = 4,
};
