///
/// bullet_common — Shared utilities implementation
///

#include <algorithm>
#include <cstdint>

#include "bullet_common.h"

#include "util/math_utils.h"

namespace bullet_common {

BulletPattern DecodePattern(uint8_t command) {
  switch (command & kCmdMask) {
  case 1:
    return BulletPattern::Circle;
  case 2:
    return BulletPattern::Random;
  default:
    return BulletPattern::Spread;
  }
}

float CalcSpreadAngle(int i, BulletPattern pattern, int n, float base_angle,
                      int dw) {
  if (n == 0) {
    return base_angle;
  }

  switch (pattern) {
  case BulletPattern::Spread: {
    i++;
    const auto direction = 1 - ((i & 1) << 1);
    const auto offset = static_cast<int>(i >> 1) * dw * direction;
    const auto centered = (n & 1) != 0 ? offset : offset - (dw >> 1);
    return base_angle + static_cast<float>(centered) * math::kLegacyAngleStep;
  }
  case BulletPattern::Circle:
    return base_angle +
           (static_cast<float>(i) * math::kFullAngle / static_cast<float>(n));
  case BulletPattern::Random: {
    const auto offset = dw == 0 ? 0 : (math::RandomInt() % dw) - (dw >> 1);
    return base_angle + static_cast<float>(offset) * math::kLegacyAngleStep;
  }
  }
  return base_angle;
}

// — Difficulty scaling ————————————————————————————

void ApplyEasyCountSpread(BulletPattern pattern, int &n, int &dw) {
  switch (pattern) {
  case BulletPattern::Spread:
    if (n >= 3) {
      n -= 2;
    }
    dw += (dw >> 2);
    return;
  case BulletPattern::Circle:
  case BulletPattern::Random:
    n >>= 1;
    return;
  }
}

void ApplyHardCountSpread(BulletPattern pattern, int &n, int &dw) {
  switch (pattern) {
  case BulletPattern::Spread:
    n += 2;
    dw -= (dw >> 3);
    return;
  case BulletPattern::Circle:
    n += std::min<int>(n >> 2, 6);
    return;
  case BulletPattern::Random:
    n += (n >> 1);
    return;
  }
}

void ApplyLunaticCountSpread(BulletPattern pattern, int &n, int &dw) {
  switch (pattern) {
  case BulletPattern::Spread:
    n += 4;
    dw -= (dw / 3);
    return;
  case BulletPattern::Circle:
    n += std::min<int>(n / 3, 12);
    return;
  case BulletPattern::Random:
    n *= 2;
    return;
  }
}

WorldCoord ScaleLengthEasy(WorldCoord length) {
  return length - length.DivPow2Floor(2);
}

WorldCoord ScaleLengthHard(WorldCoord length) {
  return length + length.DivPow2Floor(2);
}

WorldCoord ScaleLengthLunatic(WorldCoord length) {
  return length + length.DivPow2Floor(1);
}

void ApplyEasyRapid(int &ns) {
  if (ns >= 2) {
    ns--;
  }
}

void ApplyHardRapid(int &ns) { ns++; }

void ApplyLunaticRapid(int &ns) { ns += 2; }

float ScaleVelocityByRank(float v, int rank) {
  return (v * 0.5F * static_cast<float>(rank) / 8192.0F) + (v * 0.5F);
}

} // namespace bullet_common
