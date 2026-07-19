///
/// LaserHoming - Snake-like homing laser
///

#pragma once

#include "laser_base.h"

#include "core/point.h"

// ── Homing laser spawn info (ECL fills Lasers.homing_cmd) ───────
struct HomingLaserInfo {
  int x, y;
  uint8_t d;
  uint8_t dw;
  uint8_t n;
  uint8_t c;
  uint8_t type;
};

// ── Constants ───────────────────────────────────────────────────
inline constexpr auto HLASER_MAX     = 162;
inline constexpr auto HLASER_LEN     = 7;
inline constexpr auto HLASER_SECTION = 4;

inline constexpr auto HL_NONE  = 0;
inline constexpr auto HL_TYPE1 = 1;

inline constexpr auto HLS_NORM  = 0x00;
inline constexpr auto HLS_CLEAR = 0x01;
inline constexpr auto HLS_DEAD  = 0xff;

// ── LaserHoming ─────────────────────────────────────────────────
struct LaserHoming : LaserBase {
  int Current;
  int a;
  uint8_t Left;
  DegPoint p[HLASER_LEN * HLASER_SECTION];

  void Move() override;
  void Draw() const override;
  void HitCheck() override;
  bool IsDead() const override;
  void StartClear() override;
};
