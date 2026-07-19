///
/// bullet_common — Shared utilities implementation
///

#include <algorithm>

#include "bullet_common.h"

#include "util/ut_math.h"

namespace bullet_common {

uint8_t CalcSpreadDir(uint16_t i, uint8_t cmd_type, uint8_t n,
                      uint8_t base_deg, uint8_t dw) {
  switch (cmd_type) {
  case kCmdWay: {
    i++;
    if ((n & 1) != 0) {
      return base_deg + ((i >> 1) * dw * (1 - ((i & 1) << 1)));
    }
    return base_deg - (dw >> 1) + ((i >> 1) * dw * (1 - ((i & 1) << 1)));
  }
  case kCmdAll: {
    return base_deg + ((i << 8) / n);
  }
  case kCmdRnd: {
    return base_deg + (rnd() % dw) - (dw >> 1);
  }
  default:
    return 0;
  }
}

// — Difficulty scaling ————————————————————————————

bool ApplyEasyCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw) {
  switch (cmd_type) {
  case kCmdWay:
    if (n >= 3) {
      n -= 2;
    }
    dw += (dw >> 2);
    return true;
  case kCmdAll:
  case kCmdRnd:
    n >>= 1;
    return true;
  default:
    return false;
  }
}

bool ApplyHardCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw) {
  switch (cmd_type) {
  case kCmdWay:
    n += 2;
    dw -= (dw >> 3);
    return true;
  case kCmdAll:
    n += std::min<uint8_t>(n >> 2, 6);
    return true;
  case kCmdRnd:
    n += (n >> 1);
    return true;
  default:
    return false;
  }
}

bool ApplyLunaticCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw) {
  switch (cmd_type) {
  case kCmdWay:
    n += 4;
    dw -= (dw / 3);
    return true;
  case kCmdAll:
    n += std::min<uint8_t>(n / 3, 12);
    return true;
  case kCmdRnd:
    n <<= 1;
    return true;
  default:
    return false;
  }
}

int ScaleLengthEasy(int l) {
  return l - (l >> 2);
}

int ScaleLengthHard(int l) {
  return l + (l >> 2);
}

int ScaleLengthLunatic(int l) {
  return l + (l >> 1);
}

void ApplyEasyRapid(uint8_t &ns) {
  if (ns >= 2) {
    ns--;
  }
}

void ApplyHardRapid(uint8_t &ns) {
  ns++;
}

void ApplyLunaticRapid(uint8_t &ns) {
  ns += 2;
}

int ScaleVelocityByRank(int v, int rank) {
  return (((v >> 1) * rank) >> (5 + 8)) + (v >> 1);
}

} // namespace bullet_common
