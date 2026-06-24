///
/// HomingLaserSubsystem - implementation (migrated from homing_laser.cpp).
///

#include <utility>

#include "homing_laser.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "effect/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/ut_math.h"

namespace bullets {

namespace {
constexpr auto HOMINGL_WIDTH = (8 * 64);

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
  for (int j = 0, i = -64; j <= 8; ++j, i += 16) {
    src[j].x = (x + cosl(d + i, r)) >> 6;
    src[j].y = (y + sinl(d + i, r)) >> 6;
  }
  src[9] = src[0];
  gp.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, src);
}
} // namespace

HomingLaserSubsystem::HomingLaserSubsystem(world::Refs w) : world_(w) {}

void HomingLaserSubsystem::Init() {
  count_ = 0;
  active_.Next = nullptr;
  free_.Next = buf_.data();
  for (int i = 0; i < HLASER_MAX - 2; ++i)
    buf_[i].Next = &buf_[i + 1];
  buf_[HLASER_MAX - 1].Next = nullptr;
}

void HomingLaserSubsystem::SpawnHoming(const HomingLaserInfo &hinfo) {
  for (int i = 1; std::cmp_less_equal(i, hinfo.n); ++i) {
    auto *p = free_.Next;
    if (p == nullptr)
      return;

    free_.Next = free_.Next->Next;
    p->Next = active_.Next;
    active_.Next = p;
    ++count_;

    p->v = 64 * 4;
    p->a = 10;
    p->Count = 0;
    p->Current = 0;
    p->Left = 1;
    p->c = hinfo.c;
    p->Type = hinfo.type;
    p->State = HLS_NORM;

    uint8_t deg;
    if ((hinfo.n & 1) != 0) {
      deg = hinfo.d + ((i >> 1) * (hinfo.dw) * (1 - ((i & 1) << 1)));
    } else {
      deg = hinfo.d - ((hinfo.dw) >> 1) +
            ((i >> 1) * (hinfo.dw) * (1 - ((i & 1) << 1)));
    }

    for (int j = 0; j < HLASER_LEN * HLASER_SECTION; ++j) {
      p->p[j].x = hinfo.x;
      p->p[j].y = hinfo.y;
      p->p[j].d = deg;
    }
  }
}

void HomingLaserSubsystem::MoveHoming() {
  int x, y, i, j, deg, deg2;
  HomingLaserData *hl, *temp;

  for (hl = active_.Next; hl != nullptr; hl = hl->Next) {
    x = hl->p[hl->Current].x;
    y = hl->p[hl->Current].y;
    deg = hl->p[hl->Current].d;

    hl->Count++;
    hl->Current = HLASER_GETNEXT(hl->Current);

    switch (hl->Type) {
    case HL_TYPE1:
      deg2 = -deg + atan8(world_.players.X() - x, world_.players.Y() - y);
      if (deg2 < -128)
        deg2 += 256;
      else if (deg2 > 128)
        deg2 -= 256;

      if (abs(deg2) < 8) {
        hl->Type = HL_NONE;
        Snd_SEPlay(17, hl->p[hl->Current].x);
      } else {
        if (hl->v > 2 * 64)
          hl->v -= hl->a;
        i = 1 + ((hl->Count) / 32);
        i = (deg2 * i) / 32;
        deg = (i != 0) ? (deg + i) : (deg + deg2);
      }
      if (hl->Count > 120)
        hl->Type = HL_NONE;

      hl->p[hl->Current].d = deg;
      hl->p[hl->Current].x = x + cosl(deg, hl->v);
      hl->p[hl->Current].y = y + sinl(deg, hl->v);
      break;
    case HL_NONE:
      hl->v += hl->a * 2;
      hl->p[hl->Current].d = deg;
      hl->p[hl->Current].x = x + cosl(deg, hl->v);
      hl->p[hl->Current].y = y + sinl(deg, hl->v);
      break;
    default:
      break;
    }

    i = HLASER_GETNEXT(hl->Current);
    x = hl->p[i].x;
    y = hl->p[i].y;
    if (x < GX_MIN - (4 * 64) || x > GX_MAX + (4 * 64) ||
        y < GY_MIN - (4 * 64) || y > GY_MAX + (4 * 64)) {
      hl->State = HLS_DEAD;
      continue;
    }
    if (world_.players.IsInvincible() != 0U)
      continue;

    auto ev_flag = false;
    for (j = 0; j < HLASER_LEN * HLASER_SECTION; ++j) {
      x = hl->p[j].x;
      y = hl->p[j].y;
      if (HITCHK(x, world_.players.X(), HOMINGL_WIDTH + (15 * 64)) &&
          HITCHK(y, world_.players.Y(), HOMINGL_WIDTH + (15 * 64))) {
        ev_flag = true;
      }
      if (HITCHK(x, world_.players.X(), HOMINGL_WIDTH * 2 / 3) &&
          HITCHK(y, world_.players.Y(), HOMINGL_WIDTH * 2 / 3)) {
        world_.players.OnHit();
      }
    }
    if (ev_flag)
      world_.players.AddEvade(1);
  }

  for (hl = &active_; (hl->Next) != nullptr;) {
    if (hl->Next->State == HLS_DEAD) {
      temp = hl->Next->Next;
      hl->Next->Next = free_.Next;
      free_.Next = hl->Next;
      hl->Next = temp;
      --count_;
    } else {
      hl = hl->Next;
    }
  }
}

void HomingLaserSubsystem::DrawHoming() const {
  VERTEX_XY src[4];
  auto *gp = GrpGeom_Poly();
  auto *gf = GrpGeom_FB();
  const auto AlphaPolygon = [gp, gf](VERTEX_XY_SPAN<> p) {
    if (gp)
      gp->DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, p);
    else if (gf)
      gf->DrawTriangleFan(p);
  };

  if (gp != nullptr) {
    gp->SetColor({1, 2, 5});
    gp->SetAlphaOne();
  } else if (gf != nullptr) {
    gf->SetColor({2, 2, 5});
  }

  GrpGeom->Lock();

  int i, w, current;
  DegPoint *p;

  for (HomingLaserData *hl = active_.Next; hl != nullptr; hl = hl->Next) {
    w = HOMINGL_WIDTH;
    current = hl->Current;
    p = &(hl->p[current]);
    src[0].x = (p->x + cosl(p->d - 64, w)) >> 6;
    src[0].y = (p->y + sinl(p->d - 64, w)) >> 6;
    src[1].x = (p->x - cosl(p->d - 64, w)) >> 6;
    src[1].y = (p->y - sinl(p->d - 64, w)) >> 6;

    if (gp != nullptr)
      CircleA16(*gp, p->x, p->y, w, p->d);
    else
      GeomCircleF({(p->x >> 6), (p->y >> 6)}, (w >> 6));

    for (i = 0; i < HLASER_LEN - 1; ++i) {
      current = HLASER_GETPREV(current, HLASER_SECTION);
      p = &(hl->p[current]);
      src[2].x = (p->x - cosl(p->d - 64, w)) >> 6;
      src[2].y = (p->y - sinl(p->d - 64, w)) >> 6;
      src[3].x = (p->x + cosl(p->d - 64, w)) >> 6;
      src[3].y = (p->y + sinl(p->d - 64, w)) >> 6;
      AlphaPolygon(src);
      src[0] = src[3];
      src[1] = src[2];
      if (w > 64 * 2)
        w -= 64;
    }
  }

  if (gp != nullptr)
    gp->SetColor({3, 4, 5});
  else if (gf != nullptr)
    gf->SetColor({5, 5, 5});

  for (HomingLaserData *hl = active_.Next; hl != nullptr; hl = hl->Next) {
    w = HOMINGL_WIDTH / 2;
    current = hl->Current;
    p = &(hl->p[current]);
    src[0].x = (p->x + cosl(p->d - 64, w)) >> 6;
    src[0].y = (p->y + sinl(p->d - 64, w)) >> 6;
    src[1].x = (p->x - cosl(p->d - 64, w)) >> 6;
    src[1].y = (p->y - sinl(p->d - 64, w)) >> 6;

    if (gp != nullptr)
      CircleA16(*gp, p->x, p->y, w, p->d);
    else
      GeomCircleF({(p->x >> 6), (p->y >> 6)}, (w >> 6));

    for (i = 0; i < HLASER_LEN - 1; ++i) {
      current = HLASER_GETPREV(current, HLASER_SECTION);
      p = &(hl->p[current]);
      src[2].x = (p->x - cosl(p->d - 64, w)) >> 6;
      src[2].y = (p->y - sinl(p->d - 64, w)) >> 6;
      src[3].x = (p->x + cosl(p->d - 64, w)) >> 6;
      src[3].y = (p->y + sinl(p->d - 64, w)) >> 6;
      AlphaPolygon(src);
      src[0] = src[3];
      src[1] = src[2];
      if (w > 64)
        w -= 64;
      else
        break;
    }
  }

  GrpGeom->Unlock();
}

void HomingLaserSubsystem::ClearHoming() { Init(); }

} // namespace bullets