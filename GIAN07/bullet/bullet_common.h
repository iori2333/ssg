///
/// bullet_common — Shared utilities for bullet and laser systems
///

#pragma once

#include <cstdint>

#include "gfx/coords.h"

enum class HitResult : uint8_t { Miss, Graze, Hit };
inline constexpr auto kBulletEvadeValue = 1;

enum class BulletPattern : uint8_t { Spread, Circle, Random };

namespace bullet_common {

inline constexpr auto kZSet = 0x08;

inline constexpr auto kCmdMask = 0x03;

[[nodiscard]] inline int DecodeSpeed(uint8_t value) {
  return static_cast<int>(value & 0x3f) * kWorldCoordScale / 4;
}

[[nodiscard]] BulletPattern DecodePattern(uint8_t command);

// — Direction calculation ————————————————————

// Calculate the direction for bullet/laser i of n. base_angle includes the
// player-aim offset (ZSET) if applicable.
[[nodiscard]] float CalcSpreadAngle(uint16_t i, BulletPattern pattern,
                                    uint8_t n, float base_angle, uint8_t dw);

// — Difficulty scaling ——————————————————————

// Common: modifies n and dw based on difficulty level.
// Returns true if the command type was recognized.
void ApplyEasyCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw);
void ApplyHardCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw);
void ApplyLunaticCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw);

// Reflect laser length scaling.
[[nodiscard]] int ScaleLengthEasy(int l);
[[nodiscard]] int ScaleLengthHard(int l);
[[nodiscard]] int ScaleLengthLunatic(int l);

// Bullet rapid-fire count scaling.
void ApplyEasyRapid(uint8_t &ns);
void ApplyHardRapid(uint8_t &ns);
void ApplyLunaticRapid(uint8_t &ns);

// Bullet velocity scaling by rank: (v/2)*rank/8192 + v/2
// Only used for normal bullet motion.
[[nodiscard]] float ScaleVelocityByRank(float v, int rank);

} // namespace bullet_common
