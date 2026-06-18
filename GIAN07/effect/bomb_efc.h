///
/// BombEfc - Explosion effects
///

#pragma once

#include <cstdint>

// [Constants]
inline constexpr auto EXBOMB_MAX = 3;      // Max concurrent effects
inline constexpr auto EXBOMB_STD = 0;      // Common explosion type
inline constexpr auto EXBOMB_OBJMAX = 200; // Number of helper effect objects

// [Structs]
struct SpObj {
  int x, y;
  int vx, vy;
  uint8_t d;
};

struct BombEffectCtrl {
  int x, y;       // Center coordinates of the effect
  bool bIsUsed;   // Whether this struct is in use
  uint32_t count; // Frame counter

  SpObj Obj[EXBOMB_OBJMAX]; // Helper objects for effects

  uint8_t type; // Effect type
};
using BombEfcCtrl = BombEffectCtrl;

// [Function prototypes]
// Backward-compat inline wrappers moved to end of effect_manager.h
