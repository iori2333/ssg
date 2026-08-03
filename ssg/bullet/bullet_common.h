///
/// bullet_common — Shared utilities for bullet and laser systems
///

#pragma once

#include <cmath>
#include <cstdint>

#include "gfx/core/coords.h"

enum class HitResult : uint8_t { Miss, Graze, Hit };
inline constexpr auto kBulletEvadeValue = 1;

enum class BulletPattern : uint8_t { Spread, Circle, Random };

namespace bullet_common {

// Bullet and laser integrators retain the original floating-point values in
// raw world units. Keep conversion to that private representation centralized.
[[nodiscard]] inline float ToSimulationUnits(WorldCoord value) {
  return static_cast<float>(value.Raw());
}

[[nodiscard]] inline WorldCoord FromSimulationUnits(int value) {
  return WorldCoord::FromRaw(value);
}

[[nodiscard]] inline WorldCoord RoundSimulationUnits(float value) {
  return WorldCoord::FromRaw(static_cast<int>(std::lround(value)));
}

inline constexpr auto kZSet = 0x08;

inline constexpr auto kCmdMask = 0x03;

[[nodiscard]] inline int DecodeSpeed(uint8_t value) {
  return static_cast<int>(value & 0x3f) * kWorldCoordScale / 4;
}

[[nodiscard]] BulletPattern DecodePattern(uint8_t command);

// — Direction calculation ————————————————————

// Calculate the direction for bullet/laser i of n. base_angle includes the
// player-aim offset (ZSET) if applicable.
[[nodiscard]] float CalcSpreadAngle(int i, BulletPattern pattern, int n,
                                    float base_angle, int dw);

// — Difficulty scaling ——————————————————————

// Common: modifies n and dw based on difficulty level.
// Returns true if the command type was recognized.
void ApplyEasyCountSpread(BulletPattern pattern, int &n, int &dw);
void ApplyHardCountSpread(BulletPattern pattern, int &n, int &dw);
void ApplyLunaticCountSpread(BulletPattern pattern, int &n, int &dw);

// Reflect laser length scaling.
[[nodiscard]] WorldCoord ScaleLengthEasy(WorldCoord length);
[[nodiscard]] WorldCoord ScaleLengthHard(WorldCoord length);
[[nodiscard]] WorldCoord ScaleLengthLunatic(WorldCoord length);

// Bullet rapid-fire count scaling.
void ApplyEasyRapid(int &ns);
void ApplyHardRapid(int &ns);
void ApplyLunaticRapid(int &ns);

// Bullet velocity scaling by rank: (v/2)*rank/8192 + v/2
// Only used for normal bullet motion.
[[nodiscard]] float ScaleVelocityByRank(float v, int rank);

} // namespace bullet_common
