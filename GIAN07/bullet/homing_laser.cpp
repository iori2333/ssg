///
/// HomingLaser - Homing laser processing
///

#include <utility>

#include "homing_laser.h"
#include "laser_manager.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "gfx/graphics_backend.h"
#include "gfx/geometry.h"
#include "player/player.h"
#include "util/ut_math.h"

namespace {
static constexpr auto HOMINGL_WIDTH = (8 * 64);

constexpr int HLASER_GETNEXT(int current) {
  return (current + (HLASER_LEN * HLASER_SECTION) - 1) %
         (HLASER_LEN * HLASER_SECTION);
}

constexpr int HLASER_GETPREV(int current, int n) {
  return (current + n) % (HLASER_LEN * HLASER_SECTION);
}

void CircleA16(GRAPHICS_GEOMETRY_POLY auto &gp, int x, int y, int r,
               uint8_t d) {
  VERTEX_XY src[9 + 1];
  for (int j = 0, i = -64; j <= 8; j++) {
    src[j].x = (x + cosl(d + i, r)) >> 6;
    src[j].y = (y + sinl(d + i, r)) >> 6;
    i += 16;
  }
  src[9] = src[0];
  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}

} // namespace

void LaserHoming::Move() {
  // Save previous head
  int prev_x = p[Current].x;
  int prev_y = p[Current].y;
  int prev_deg = p[Current].d;

  // Common update
  count++;
  Current = HLASER_GETNEXT(Current);

  switch (type) {
  case HL_TYPE1: {
    int deg2 = -prev_deg + atan8(Players.X() - prev_x, Players.Y() - prev_y);
    if (deg2 < -128) deg2 += 256;
    else if (deg2 > 128) deg2 -= 256;

    if (abs(deg2) < 8) {
      type = HL_NONE;
      Snd_SEPlay(static_cast<SfxId>(17), p[Current].x);
    } else {
      if (v > 2 * 64) v -= a;
      int i = 1 + (static_cast<int>(count) / 32);
      i = (deg2 * i) / 32;
      if (i != 0) prev_deg = prev_deg + i;
      else prev_deg = prev_deg + deg2;
    }

    if (count > 120) type = HL_NONE;

    p[Current].d = prev_deg;
    p[Current].x = prev_x + cosl(prev_deg, v);
    p[Current].y = prev_y + sinl(prev_deg, v);
    break;
  }

  case HL_NONE:
    v += a * 2;
    p[Current].d = prev_deg;
    p[Current].x = prev_x + cosl(prev_deg, v);
    p[Current].y = prev_y + sinl(prev_deg, v);
    break;

  default:
    break;
  }

  // Out-of-range check
  int tail_i = HLASER_GETNEXT(Current);
  int tx = p[tail_i].x;
  int ty = p[tail_i].y;
  if (tx < GX_MIN - (4 * 64) || tx > GX_MAX + (4 * 64) ||
      ty < GY_MIN - (4 * 64) || ty > GY_MAX + (4 * 64)) {
    flag = HLS_DEAD;
  }
}

void LaserHoming::Draw() const {
  const auto AlphaPolygon = [](VERTEX_XY_SPAN<4> src) {
    if (auto *gp = GrpGeom_Poly()) {
      gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
    } else if (auto *gf = GrpGeom_FB()) {
      gf->DrawTriangleFan(src);
    }
  };

  auto *gp = GrpGeom_Poly();
  auto *gf = GrpGeom_FB();

  // Pass 1: wide outer ribbon (blue)
  if (gp != nullptr) {
    gp->SetColor({1, 2, 5});
    gp->SetAlphaOne();
  } else if (gf != nullptr) {
    gf->SetColor({2, 2, 5});
  }

  int w = HOMINGL_WIDTH;
  int cur = Current;
  const DegPoint *pt = &p[cur];

  VERTEX_XY src[4];
  src[0].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
  src[0].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
  src[1].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
  src[1].y = (pt->y - sinl(pt->d - 64, w)) >> 6;

  if (gp != nullptr) {
    CircleA16(*gp, pt->x, pt->y, w, pt->d);
  } else {
    GeomCircleF({(pt->x >> 6), (pt->y >> 6)}, (w >> 6));
  }

  for (int i = 0; i < HLASER_LEN - 1; i++) {
    cur = HLASER_GETPREV(cur, HLASER_SECTION);
    pt = &p[cur];

    src[2].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
    src[2].y = (pt->y - sinl(pt->d - 64, w)) >> 6;
    src[3].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
    src[3].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
    AlphaPolygon(src);

    src[0] = src[3];
    src[1] = src[2];

    if (w > 64 * 2) w -= 64;
  }

  // Pass 2: narrow inner highlight (white)
  if (gp != nullptr) {
    gp->SetColor({3, 4, 5});
  } else if (gf != nullptr) {
    gf->SetColor({5, 5, 5});
  }

  w = HOMINGL_WIDTH / 2;
  cur = Current;
  pt = &p[cur];

  src[0].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
  src[0].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
  src[1].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
  src[1].y = (pt->y - sinl(pt->d - 64, w)) >> 6;

  if (gp != nullptr) {
    CircleA16(*gp, pt->x, pt->y, w, pt->d);
  } else {
    GeomCircleF({(pt->x >> 6), (pt->y >> 6)}, (w >> 6));
  }

  for (int i = 0; i < HLASER_LEN - 1; i++) {
    cur = HLASER_GETPREV(cur, HLASER_SECTION);
    pt = &p[cur];

    src[2].x = (pt->x - cosl(pt->d - 64, w)) >> 6;
    src[2].y = (pt->y - sinl(pt->d - 64, w)) >> 6;
    src[3].x = (pt->x + cosl(pt->d - 64, w)) >> 6;
    src[3].y = (pt->y + sinl(pt->d - 64, w)) >> 6;
    AlphaPolygon(src);

    src[0] = src[3];
    src[1] = src[2];

    if (w > 64) w -= 64;
    else break;
  }
}

void LaserHoming::HitCheck() {
  if (Players.IsInvincible() != 0U) return;

  bool ev_flag = false;
  for (auto & j : p) {
    const int hx = j.x;
    const int hy = j.y;

    if (HITCHK(hx, Players.X(), HOMINGL_WIDTH + (15 * 64)) &&
        HITCHK(hy, Players.Y(), HOMINGL_WIDTH + (15 * 64))) {
      ev_flag = true;
    }

    if (HITCHK(hx, Players.X(), (HOMINGL_WIDTH * 2 / 3) + PLAYER_HITBOX_RADIUS) &&
        HITCHK(hy, Players.Y(), (HOMINGL_WIDTH * 2 / 3) + PLAYER_HITBOX_RADIUS)) {
      Players.OnHit();
    }
  }
  if (ev_flag) Players.AddEvade(1);
}

bool LaserHoming::IsDead() const {
  return flag == HLS_DEAD;
}

void LaserHoming::StartClear() {
  flag = HLS_DEAD;
}

// ── LaserManager homing-laser public methods ────────────────────

void LaserManager::InitHoming() {
  homing.Init();
}

void LaserManager::SpawnHoming(const HomingLaserInfo *hinfo) {
  uint8_t deg = 0;

  for (int i = 1; i <= (hinfo->n); i++) {
    auto *p = homing.Alloc();
    if (p == nullptr) return;

    p->v = 64 * 4;
    p->a = 10;
    p->count = 0;
    p->Current = 0;
    p->Left = 1;

    p->c = hinfo->c;
    p->type = hinfo->type;
    p->flag = HLS_NORM;

    if ((hinfo->n & 1) != 0) {
      deg = hinfo->d + ((i >> 1) * (hinfo->dw) * (1 - ((i & 1) << 1)));
    } else {
      deg = hinfo->d - ((hinfo->dw) >> 1) +
            ((i >> 1) * (hinfo->dw) * (1 - ((i & 1) << 1)));
    }

    for (auto & j : p->p) {
      j.x = hinfo->x;
      j.y = hinfo->y;
      j.d = deg;
    }
  }
}

void LaserManager::MoveHoming() {
  for (uint16_t i = 0; i < homing.count; i++) {
    auto *hl = &homing.Active(i);
    hl->Move();           // virtual
    if (hl->flag != HLS_DEAD)
      hl->HitCheck();     // virtual
  }
  homing.Compact([](const LaserHoming &h) { return h.flag == HLS_DEAD; });
}

void LaserManager::DrawHoming() const {
  GrpGeom->Lock();

  for (uint16_t i = 0; i < homing.count; i++)
    homing.Active(i).Draw();

  GrpGeom->Unlock();
}

void LaserManager::ClearHoming() {
  homing.Init();
}
