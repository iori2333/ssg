///
/// LaserLong - Infinite-distance beam anchored to an enemy
///

#pragma once

#include "laser_base.h"

#include "enemy/enemy.h"
#include "gfx/graphics_backend.h"

// ── Long laser command (ECL fills Lasers.long_cmd before SpawnLongLaser) ──
struct LongLaserCommand {
  EnemyData *e;
  int dx, dy;
  int v;
  int w;
  uint8_t d;
  uint8_t c;
  uint8_t type;
};

// ── Constants ───────────────────────────────────────────────────
inline constexpr auto LLASER_MAX = 20;
inline constexpr auto LLASER_EVADE = 1;

inline constexpr auto LLS_LONG   = 0x00;
inline constexpr auto LLS_LONGY  = 0x01;
inline constexpr auto LLS_SETDEG = 0x02;
inline constexpr auto LLS_LONGZ  = 0x03;

inline constexpr auto LLF_DISABLE = 0x00;
inline constexpr auto LLF_NORM    = 0x01;
inline constexpr auto LLF_OPEN    = 0x02;
inline constexpr auto LLF_CLOSE   = 0x04;
inline constexpr auto LLF_CLOSEL  = 0x08;
inline constexpr auto LLF_LINE    = 0x10;

// ── LaserLong ───────────────────────────────────────────────────
struct LaserLong : LaserBase {
  EnemyData *e;
  int dx, dy;
  int lx, ly;
  int infx, infy;
  int wx, wy;
  int w, wmax;
  VERTEX_XY p[4];
  uint8_t EnemyID;

  void Move() override;
  void Draw() const override;
  void HitCheck() override;
  bool IsDead() const override;
  void StartClear() override;
};
