///
/// LaserReflect - Short / reflective laser
///

#pragma once

#include "laser_base.h"

#include "gfx/graphics_backend.h"

// ── Laser command (ECL fills Lasers.cmd before Spawn) ───────────
struct LaserCommand {
  int x, y;
  int v;
  int w, l, l2;
  uint8_t d, dw;
  uint8_t n, c;
  char   a;
  uint8_t cmd, type, notr;
};

// ── Constants ───────────────────────────────────────────────────
inline constexpr auto LASER_MAX = 1000;
inline constexpr auto LF_DELETE = 0x80;

// ── LaserReflect ────────────────────────────────────────────────
struct LaserReflect : LaserBase {
  int vx, vy;
  int lx, ly;
  int wx, wy;
  VERTEX_XY p[4];
  char  a;
  int   w, wmax;
  int   l, lmax;
  int   ltemp;
  uint8_t notr;
  uint8_t evade;

  void Move() override;
  void Draw() const override;
  void HitCheck() override;
  bool IsDead() const override;
  void StartClear() override;

  // geometry helper used by LaserManager during spawn
  void SetupShort();
};
