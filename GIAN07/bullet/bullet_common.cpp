///
/// bullet_common — Shared utilities implementation
///

#include <algorithm>

#include "bullet_common.h"

#include "util/ut_math.h"

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

uint8_t CalcSpreadDir(uint16_t i, BulletPattern pattern, uint8_t n,
                      uint8_t base_deg, uint8_t dw) {
  switch (pattern) {
  case BulletPattern::Spread: {
    i++;
    if ((n & 1) != 0) {
      return base_deg + ((i >> 1) * dw * (1 - ((i & 1) << 1)));
    }
    return base_deg - (dw >> 1) + ((i >> 1) * dw * (1 - ((i & 1) << 1)));
  }
  case BulletPattern::Circle: {
    return base_deg + ((i << 8) / n);
  }
  case BulletPattern::Random: {
    return base_deg + (rnd() % dw) - (dw >> 1);
  }
  }
  return base_deg;
}

// — Difficulty scaling ————————————————————————————

void ApplyEasyCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw) {
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

void ApplyHardCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw) {
  switch (pattern) {
  case BulletPattern::Spread:
    n += 2;
    dw -= (dw >> 3);
    return;
  case BulletPattern::Circle:
    n += std::min<uint8_t>(n >> 2, 6);
    return;
  case BulletPattern::Random:
    n += (n >> 1);
    return;
  }
}

void ApplyLunaticCountSpread(BulletPattern pattern, uint8_t &n, uint8_t &dw) {
  switch (pattern) {
  case BulletPattern::Spread:
    n += 4;
    dw -= (dw / 3);
    return;
  case BulletPattern::Circle:
    n += std::min<uint8_t>(n / 3, 12);
    return;
  case BulletPattern::Random:
    n <<= 1;
    return;
  }
}

int ScaleLengthEasy(int l) { return l - (l >> 2); }

int ScaleLengthHard(int l) { return l + (l >> 2); }

int ScaleLengthLunatic(int l) { return l + (l >> 1); }

void ApplyEasyRapid(uint8_t &ns) {
  if (ns >= 2) {
    ns--;
  }
}

void ApplyHardRapid(uint8_t &ns) { ns++; }

void ApplyLunaticRapid(uint8_t &ns) { ns += 2; }

int ScaleVelocityByRank(int v, int rank) {
  return (((v >> 1) * rank) >> (5 + 8)) + (v >> 1);
}

} // namespace bullet_common
