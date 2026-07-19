///
/// LaserReflect - Short / reflective laser member functions
///

#include "laser_reflect.h"
#include "laser_manager.h"
#include "long_laser.h"

#include "core/gian.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "player/player.h"
#include "util/ut_math.h"

// Laser geometry:
//   3-----------------Length---> >----------2
//  Width                      < <             |
//   +(x,y)                     > >            +
//  Width                      < <             |
//   0-----------------Length---> >----------1

// ── local constants ────────────────────────────────────────────
static constexpr auto LS_SHORT = 0x00;
static constexpr auto LS_REF   = 0x01;

static constexpr auto LF_NONE  = 0x00;
static constexpr auto LF_CLEAR = 0x01;
static constexpr auto LF_SHOT  = 0x02;
static constexpr auto LF_HIT   = 0x04;
static constexpr auto LF_NMOVE = 0x06;

// ── LaserReflect member functions ──────────────────────────────

void LaserReflect::SetupShort() {
  p[1].x = p[0].x = (x >> 6) + wx;
  p[1].y = p[0].y = (y >> 6) + wy;

  p[2].x = p[3].x = (x >> 6) - wx;
  p[2].y = p[3].y = (y >> 6) - wy;

  p[1].x += lx;
  p[1].y += ly;
  p[2].x += lx;
  p[2].y += ly;
}

void LaserReflect::Move() {
  if (flag == LF_CLEAR) {
    if (l < lmax) {
      l += v;
      w += 16;
      lx = cosl(d, l >> 6);
      ly = sinl(d, l >> 6);
      p[1].x = p[0].x + lx;
      p[1].y = p[0].y + ly;
      p[2].x = p[3].x + lx;
      p[2].y = p[3].y + ly;
    } else {
      w += 64;
    }

    wx = -sinl(d, w >> 6);
    wy =  cosl(d, w >> 6);
    SetupShort();

    if (count > 30) {
      flag = LF_DELETE;
    }
    return;
  }

  switch (type) {
  case LS_SHORT:
    if (l < lmax) {
      l += v;
      lx = cosl(d, l >> 6);
      ly = sinl(d, l >> 6);
      p[1].x = p[0].x + lx;
      p[1].y = p[0].y + ly;
      p[2].x = p[3].x + lx;
      p[2].y = p[3].y + ly;
    } else {
      x += vx;
      y += vy;
      SetupShort();
    }
    return;

  case LS_REF: {
    switch (flag) {
    case LF_NONE:
      x += vx;
      y += vy;
      SetupShort();
      if (Lasers.HitReflect(this) != 0) { flag = LF_HIT; }
      return;

    case LF_SHOT:
      l += v;
      lx = cosl(d, l >> 6);
      ly = sinl(d, l >> 6);
      p[1].x = p[0].x + lx;
      p[1].y = p[0].y + ly;
      p[2].x = p[3].x + lx;
      p[2].y = p[3].y + ly;
      if (l >= lmax) { flag = LF_NONE; }
      if (Lasers.HitReflect(this) != 0) {
        ltemp = l;
        flag |= LF_HIT;
      }
      return;

    case LF_HIT:
      if (l <= v) {
        flag = LF_DELETE;
        return;
      }
      l -= v;
      x += vx;
      y += vy;
      lx = cosl(d, l >> 6);
      ly = sinl(d, l >> 6);
      p[0].x = p[1].x - lx;
      p[0].y = p[1].y - ly;
      p[3].x = p[2].x - lx;
      p[3].y = p[2].y - ly;
      return;

    case LF_NMOVE:
      ltemp += v;
      if (ltemp >= lmax) { flag = LF_HIT; }
      return;
    }
  }
  }
}

void LaserReflect::Draw() const {
  constexpr RGB216 col = {1, 0, 5};

  if (flag == LF_CLEAR) {
    GrpGeom->SetColor(col);
    GrpGeom->DrawLine(p[0].x, p[0].y, p[1].x, p[1].y);
    GrpGeom->DrawLine(p[3].x, p[3].y, p[2].x, p[2].y);
    return;
  }

  if (auto *gp = GrpGeom_Poly()) {
    GeomGrdRect(*gp, p, col.ToRGB());
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({1, 0, 5});
    gf->DrawTriangleFan(p);

    gf->SetColor({5, 5, 5});

    VERTEX_XY inner[4];
    inner[0].x = inner[1].x = p[0].x - (wx * 3 / 4);
    inner[0].y = inner[1].y = p[0].y - (wy * 3 / 4);
    inner[3].x = inner[2].x = p[3].x + (wx * 3 / 4);
    inner[3].y = inner[2].y = p[3].y + (wy * 3 / 4);
    inner[1].x += lx;
    inner[1].y += ly;
    inner[2].x += lx;
    inner[2].y += ly;
    gf->DrawTriangleFan(inner);
  }
}

void LaserReflect::HitCheck() {
  if (type != LS_SHORT && type != LS_REF) { return; }

  static constexpr auto LASER_EVADE_WIDTH = (12 * 64);
  const int tx = Players.X() - x;
  const int ty = Players.Y() - y;
  const int length = cosl(d, tx) + sinl(d, ty);
  const int w1 = abs(-sinl(d, tx) + cosl(d, ty));

  if (length > 0 && length <= l && w1 <= (w + PLAYER_HITBOX_RADIUS)) {
    Players.OnHit();
  } else if (length > 0 && length <= l &&
             w1 <= (w + LASER_EVADE_WIDTH)) {
    if (evade != 0U) {
      Players.AddEvade(0);
    } else {
      evade = 0xff;
      Players.AddEvade(3);
    }
  }
}

bool LaserReflect::IsDead() const {
  return (flag & LF_DELETE) != 0;
}

void LaserReflect::StartClear() {
  if (flag != LF_CLEAR) {
    flag = LF_CLEAR;
    count = 0;
  }
}
