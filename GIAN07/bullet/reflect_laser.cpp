///
/// ReflectLaserSubsystem - implementation (migrated from laser.cpp).
///

#include <utility>

#include "long_laser.h"
#include "reflect_laser.h"

#include "core/entity.h"
#include "core/gian.h"
#include "core/level.h"
#include "effect/geometry.h"
#include "gameflow/play_rank.h"
#include "gfx/graphics_backend.h"
#include "util/ut_math.h"

namespace bullets {

namespace {
constexpr auto RT_MAX = 10;
constexpr auto LS_ZSET = 0x08;
constexpr auto SLASER_EVADE = 3;
constexpr auto LASER_EVADE_WIDTH = (12 * 64);

constexpr auto LS_SHORT = 0x00;
constexpr auto LS_REF = 0x01;
constexpr auto LS_LONG = 0x02;
constexpr auto LS_LONGY = 0x03;

#undef LC_ALL
constexpr auto LC_WAY = 0x00;
constexpr auto LC_ALL = 0x01;
constexpr auto LC_RND = 0x02;
constexpr auto LC_WAYZ = 0x08;
constexpr auto LC_ALLZ = 0x09;
constexpr auto LC_RNDZ = 0x0a;

constexpr auto LF_NONE = 0x00;
constexpr auto LF_CLEAR = 0x01;
constexpr auto LF_SHOT = 0x02;
constexpr auto LF_HIT = 0x04;
constexpr auto LF_NMOVE = 0x06;
} // namespace

ReflectLaserSubsystem::ReflectLaserSubsystem(world::Refs w,
                                             LongLaserSubsystem &lon)
    : world_(w), lon_(lon) {}

void ReflectLaserSubsystem::Reset() {
  for (int i = 0; i < LASER_MAX; ++i)
    indices_[i] = static_cast<uint16_t>(i);
  count_ = 0;
}

void ReflectLaserSubsystem::Spawn(const LaserCommand &cmd_in) {
  LaserCommand cmd = cmd_in;
  switch (world_.ranking.state.GameLevel) {
  case GameLevel::EASY:
    SetEasy(cmd);
    break;
  case GameLevel::NORMAL:
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    SetHard(cmd);
    break;
  case GameLevel::LUNATIC:
    SetLunatic(cmd);
    break;
  }
  cmd.v =
      (((cmd.v >> 1) * (world_.ranking.state.Rank)) >> (5 + 8)) + (cmd.v >> 1);
  SpawnEX(cmd);
}

void ReflectLaserSubsystem::SpawnEX(const LaserCommand &cmd) {
  for (uint8_t i = 0; i < cmd.n; ++i) {
    if (count_ + 1 == LASER_MAX)
      return;
    auto *lp = &lasers_[indices_[count_++]];

    lp->v = cmd.v;
    lp->a = cmd.a;
    lp->d = CalcDir(i, cmd);
    if (cmd.l2 != 0) {
      lp->x = cmd.x + cosl(lp->d, cmd.l2);
      lp->y = cmd.y + sinl(lp->d, cmd.l2);
    } else {
      lp->x = cmd.x;
      lp->y = cmd.y;
    }
    lp->vx = cosl(lp->d, lp->v);
    lp->vy = sinl(lp->d, lp->v);
    lp->w = cmd.w;
    lp->lmax = cmd.l;
    lp->lx = lp->ly = 0;
    lp->wx = -sinl(lp->d, lp->w >> 6);
    lp->wy = cosl(lp->d, lp->w >> 6);
    lp->l = lp->count = 0;
    lp->c = cmd.c;
    lp->type = cmd.type;
    lp->flag = (lp->type == LS_REF) ? LF_SHOT : LF_NONE;
    lp->evade = 0;
    lp->notr = cmd.notr;
    SetupShort(lp);
  }
}

void ReflectLaserSubsystem::Move() {
  for (uint16_t i = 0; i < count_; ++i) {
    auto *lp = &lasers_[indices_[i]];
    MoveLaser(lp);
    lp->count++;
    if ((lp->x) < GX_MIN || (lp->x) > GX_MAX || (lp->y) < GY_MIN ||
        (lp->y) > GY_MAX) {
      lp->flag = LF_DELETE;
    }
    if (world_.players.IsInvincible() == 0 &&
        ((lp->flag & (LF_CLEAR | LF_DELETE)) == 0)) {
      HitCheck(lp, world_);
    }
  }
  Indsort(indices_, count_, lasers_,
          [](const LASER_DATA &l) { return (l.flag & LF_DELETE); });
}

void ReflectLaserSubsystem::Draw() {
  GrpGeom->Lock();
  for (uint16_t i = 0; std::cmp_less(i, count_); ++i) {
    auto *lp = &lasers_[indices_[i]];
    switch (lp->type) {
    case LS_SHORT:
    case LS_REF:
      DrawShort(lp);
      break;
    default:
      break;
    }
  }
  GrpGeom->Unlock();
}

void ReflectLaserSubsystem::Clear() {
  for (uint16_t i = 0; i < count_; ++i) {
    auto &l = lasers_[indices_[i]];
    if (l.flag != LF_CLEAR) {
      l.flag = LF_CLEAR;
      l.count = 0;
    }
  }
}

std::span<const LASER_DATA> ReflectLaserSubsystem::Active() const {
  return {lasers_.data(), count_};
}

void ReflectLaserSubsystem::SetEasy(LaserCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    if (cmd.n >= 3)
      cmd.n -= 2;
    cmd.dw += (cmd.dw >> 2);
    break;
  case LC_ALL:
  case LC_RND:
    cmd.n >>= 1;
    break;
  }
  cmd.l -= (cmd.l >> 2);
}

void ReflectLaserSubsystem::SetHard(LaserCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    cmd.n += 2;
    cmd.dw -= (cmd.dw >> 3);
    break;
  case LC_ALL:
    cmd.n += (((cmd.n >> 2) > 6) ? 6 : (cmd.n >> 2));
    break;
  case LC_RND:
    cmd.n += (cmd.n >> 1);
    break;
  }
  cmd.l += (cmd.l >> 2);
}

void ReflectLaserSubsystem::SetLunatic(LaserCommand &cmd) const {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    cmd.n += 4;
    cmd.dw -= (cmd.dw / 3);
    break;
  case LC_ALL:
    cmd.n += (((cmd.n / 3) > 12) ? 12 : (cmd.n / 3));
    break;
  case LC_RND:
    cmd.n <<= 1;
    break;
  }
  cmd.l += (cmd.l >> 1);
}

uint8_t ReflectLaserSubsystem::CalcDir(uint16_t i,
                                       const LaserCommand &cmd) const {
  uint8_t deg = 0;
  if ((cmd.cmd & LS_ZSET) != 0) {
    deg = atan8(world_.players.X() - cmd.x, world_.players.Y() - cmd.y);
  }
  deg += cmd.d;
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    i++;
    if ((cmd.n & 1) != 0) {
      return deg + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
    }
    return deg - (cmd.dw >> 1) + ((i >> 1) * cmd.dw * (1 - ((i & 1) << 1)));
  case LC_ALL:
    return deg + ((i << 8) / cmd.n);
  case LC_RND:
    return deg + (rnd() % cmd.dw) - (cmd.dw >> 1);
  default:
    return 0;
  }
}

void ReflectLaserSubsystem::SetupShort(LASER_DATA *lp) {
  lp->p[1].x = lp->p[0].x = (lp->x >> 6) + lp->wx;
  lp->p[1].y = lp->p[0].y = (lp->y >> 6) + lp->wy;
  lp->p[2].x = lp->p[3].x = (lp->x >> 6) - lp->wx;
  lp->p[2].y = lp->p[3].y = (lp->y >> 6) - lp->wy;
  lp->p[1].x += lp->lx;
  lp->p[1].y += lp->ly;
  lp->p[2].x += lp->lx;
  lp->p[2].y += lp->ly;
}

void ReflectLaserSubsystem::DrawShort(const LASER_DATA *lp) {
  constexpr RGB216 col = {1, 0, 5};
  if (lp->flag == LF_CLEAR) {
    GrpGeom->SetColor(col);
    GrpGeom->DrawLine(lp->p[0].x, lp->p[0].y, lp->p[1].x, lp->p[1].y);
    GrpGeom->DrawLine(lp->p[3].x, lp->p[3].y, lp->p[2].x, lp->p[2].y);
    return;
  }

  if (auto *gp = GrpGeom_Poly()) {
    GeomGrdRect(*gp, lp->p, col.ToRGB());
  } else if (auto *gf = GrpGeom_FB()) {
    gf->SetColor({1, 0, 5});
    gf->DrawTriangleFan(lp->p);
    gf->SetColor({5, 5, 5});
    VERTEX_XY p[4];
    p[0].x = p[1].x = lp->p[0].x - (lp->wx * 3 / 4);
    p[0].y = p[1].y = lp->p[0].y - (lp->wy * 3 / 4);
    p[3].x = p[2].x = lp->p[3].x + (lp->wx * 3 / 4);
    p[3].y = p[2].y = lp->p[3].y + (lp->wy * 3 / 4);
    p[1].x += lp->lx;
    p[1].y += lp->ly;
    p[2].x += lp->lx;
    p[2].y += lp->ly;
    gf->DrawTriangleFan(p);
  }
}

void ReflectLaserSubsystem::MoveLaser(LASER_DATA *lp) {
  if (lp->flag == LF_CLEAR) {
    if (lp->l < lp->lmax) {
      lp->l += lp->v;
      lp->w += 16;
      lp->lx = cosl(lp->d, lp->l >> 6);
      lp->ly = sinl(lp->d, lp->l >> 6);
      lp->p[1].x = lp->p[0].x + lp->lx;
      lp->p[1].y = lp->p[0].y + lp->ly;
      lp->p[2].x = lp->p[3].x + lp->lx;
      lp->p[2].y = lp->p[3].y + lp->ly;
    } else {
      lp->w += 64;
    }
    lp->wx = -sinl(lp->d, lp->w >> 6);
    lp->wy = cosl(lp->d, lp->w >> 6);
    SetupShort(lp);
    if (lp->count > 30)
      lp->flag = LF_DELETE;
    return;
  }

  switch (lp->type) {
  case LS_SHORT:
    if ((lp->l) < (lp->lmax)) {
      lp->l += lp->v;
      lp->lx = cosl(lp->d, lp->l >> 6);
      lp->ly = sinl(lp->d, lp->l >> 6);
      lp->p[1].x = lp->p[0].x + lp->lx;
      lp->p[1].y = lp->p[0].y + lp->ly;
      lp->p[2].x = lp->p[3].x + lp->lx;
      lp->p[2].y = lp->p[3].y + lp->ly;
    } else {
      lp->x += lp->vx;
      lp->y += lp->vy;
      SetupShort(lp);
    }
    return;
  case LS_REF:
    MoveReflect(lp);
    return;
  default:
    break;
  }
}

void ReflectLaserSubsystem::HitCheck(LASER_DATA *lp, world::Refs w) {
  long tx = 0, ty = 0, w1 = 0, length = 0;
  switch (lp->type) {
  case LS_SHORT:
  case LS_REF:
    tx = w.players.X() - lp->x;
    ty = w.players.Y() - lp->y;
    length = cosl(lp->d, tx) + sinl(lp->d, ty);
    w1 = abs(-sinl(lp->d, tx) + cosl(lp->d, ty));
    if (length > 0 && length <= (lp->l) && w1 <= (lp->w)) {
      w.players.OnHit();
    } else if (length > 0 && length <= (lp->l) &&
               w1 <= (lp->w + LASER_EVADE_WIDTH)) {
      if (lp->evade != 0U) {
        w.players.AddEvade(0);
      } else {
        lp->evade = 0xff;
        w.players.AddEvade(SLASER_EVADE);
      }
    }
    break;
  default:
    break;
  }
}

void ReflectLaserSubsystem::MoveReflect(LASER_DATA *lp) {
  switch (lp->flag) {
  case LF_NONE:
    lp->x += lp->vx;
    lp->y += lp->vy;
    SetupShort(lp);
    if (HitReflect(lp) != 0)
      lp->flag = LF_HIT;
    return;
  case LF_SHOT:
    lp->l += lp->v;
    lp->lx = cosl(lp->d, lp->l >> 6);
    lp->ly = sinl(lp->d, lp->l >> 6);
    lp->p[1].x = lp->p[0].x + lp->lx;
    lp->p[1].y = lp->p[0].y + lp->ly;
    lp->p[2].x = lp->p[3].x + lp->lx;
    lp->p[2].y = lp->p[3].y + lp->ly;
    if ((lp->l) >= (lp->lmax))
      lp->flag = LF_NONE;
    if (HitReflect(lp) != 0) {
      lp->ltemp = lp->l;
      lp->flag |= LF_HIT;
    }
    return;
  case LF_HIT:
    if ((lp->l) <= (lp->v)) {
      lp->flag = LF_DELETE;
    } else {
      lp->l -= lp->v;
    }
    lp->x += lp->vx;
    lp->y += lp->vy;
    lp->lx = cosl(lp->d, lp->l >> 6);
    lp->ly = sinl(lp->d, lp->l >> 6);
    lp->p[0].x = lp->p[1].x - lp->lx;
    lp->p[0].y = lp->p[1].y - lp->ly;
    lp->p[3].x = lp->p[2].x - lp->lx;
    lp->p[3].y = lp->p[2].y - lp->ly;
    return;
  case LF_NMOVE:
    lp->ltemp += lp->v;
    if ((lp->ltemp) >= (lp->lmax))
      lp->flag = LF_HIT;
    return;
  }
}

int ReflectLaserSubsystem::HitReflect(const LASER_DATA *lp) {
  LongLaserData *ll = nullptr;
  const long lx = (lp->x + cosl(lp->d, lp->l));
  const long ly = (lp->y + sinl(lp->d, lp->l));
  long tx = 0, ty = 0, length = 0, width = 0;

  for (int i = 0; i < LLASER_MAX; ++i) {
    if (std::cmp_equal(i, lp->notr))
      continue;
    ll = &lon_.AllUnsafe()[i];
    if (ll->flag != LLF_NORM)
      continue;

    tx = lx - ll->x;
    ty = ly - ll->y;
    length = cosl(ll->d, tx) + sinl(ll->d, ty);
    width = abs(-sinl(ll->d, tx) + cosl(ll->d, ty));

    if (length > 0 && width <= ll->w) {
      LaserCommand cmd{};
      cmd.x = lx;
      cmd.y = ly;
      cmd.v = lp->v;
      cmd.d = -(lp->d) + ((ll->d) << 1);
      cmd.w = lp->w;
      cmd.l = lp->lmax;
      cmd.l2 = 0;
      cmd.n = 1;
      cmd.c = lp->c;
      cmd.cmd = LC_WAY;
      cmd.type = LS_REF;
      cmd.notr = static_cast<uint8_t>(i);
      SpawnEX(cmd);
      return 1;
    }
  }
  return 0;
}

} // namespace bullets