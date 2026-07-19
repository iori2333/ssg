///
/// LaserManager — reflective-laser pool management + cross-pool helpers
///

#include "laser_reflect.h"
#include "laser_manager.h"
#include "long_laser.h"

#include "core/entity.h"
#include "core/gian.h"
#include "core/level.h"
#include "gameflow/play_rank.h"
#include "gameflow/rank_manager.h"
#include "player/player.h"
#include "util/ut_math.h"

// ── local constants ────────────────────────────────────────────
static constexpr auto LS_ZSET = 0x08;
static constexpr auto LS_REF  = 0x01;

#undef LC_ALL
static constexpr auto LC_WAY  = 0x00;
static constexpr auto LC_ALL  = 0x01;
static constexpr auto LC_RND  = 0x02;
static constexpr auto LC_WAYZ = 0x08;
static constexpr auto LC_ALLZ = 0x09;
static constexpr auto LC_RNDZ = 0x0a;

static constexpr auto LF_SHOT = 0x02;
static constexpr auto LF_NONE = 0x00;

// ── LaserManager reflective-laser methods ──────────────────────

void LaserManager::Spawn() {
  switch (Ranking.state.level) {
  case GameLevel::EASY:   SetEasy();    break;
  case GameLevel::NORMAL:              break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:  SetHard();    break;
  case GameLevel::LUNATIC:SetLunatic(); break;
  }

  cmd.v = (((cmd.v >> 1) * (Ranking.state.Rank)) >> (5 + 8)) + (cmd.v >> 1);

  SpawnEX();
}

void LaserManager::SpawnEX() {
  for (decltype(cmd.n) i = 0; i < cmd.n; i++) {
    auto *lp = reflect.Alloc();
    if (lp == nullptr) { return; }

    lp->v = cmd.v;
    lp->a = cmd.a;
    lp->d = CalcDir(i);

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
    lp->wy =  cosl(lp->d, lp->w >> 6);

    lp->l = lp->count = 0;

    lp->c = cmd.c;
    lp->type = cmd.type;

    if (lp->type == LS_REF) {
      lp->flag = LF_SHOT;
    } else {
      lp->flag = LF_NONE;
    }

    lp->evade = 0;
    lp->notr = cmd.notr;

    lp->SetupShort();
  }
}

void LaserManager::SetIndices() {
  reflect.Init();
}

void LaserManager::SetEasy() {
  switch (cmd.cmd & 0x03) {
  case LC_WAY:
    if (cmd.n >= 3) { cmd.n -= 2; }
    cmd.dw += (cmd.dw >> 2);
    break;
  case LC_ALL:
  case LC_RND:
    cmd.n >>= 1;
    break;
  }
  cmd.l -= (cmd.l >> 2);
}

void LaserManager::SetHard() {
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

void LaserManager::SetLunatic() {
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

uint8_t LaserManager::CalcDir(uint16_t i) const {
  uint8_t deg = 0;

  if ((cmd.cmd & LS_ZSET) != 0) {
    deg = atan8(Players.X() - cmd.x, Players.Y() - cmd.y);
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

int LaserManager::HitReflect(const LaserReflect *lp) {
  const long lx = (lp->x + cosl(lp->d, lp->l));
  const long ly = (lp->y + sinl(lp->d, lp->l));

  for (uint16_t i = 0; i < Lasers.long_lasers.count; i++) {
    const auto *ll = &Lasers.long_lasers.Active(i);
    if (Lasers.long_lasers.RawIndex(i) == lp->notr) { continue; }
    if (ll->flag != LLF_NORM) { continue; }

    const long tx = lx - ll->x;
    const long ty = ly - ll->y;
    const long length = cosl(ll->d, tx) + sinl(ll->d, ty);
    const long width  = abs(-sinl(ll->d, tx) + cosl(ll->d, ty));

    if (length > 0 && width <= ll->w) {
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
      cmd.notr = i;
      SpawnEX();
      return 1;
    }
  }

  return 0;
}

// ── Unified per-frame entry points ─────────────────────────────

void LaserManager::MoveAll() {
  for (uint16_t i = 0; i < reflect.count; i++) {
    auto *lp = &reflect.Active(i);
    lp->Move();
    lp->count++;
    if (lp->x < GX_MIN || lp->x > GX_MAX || lp->y < GY_MIN || lp->y > GY_MAX) {
      lp->flag = LF_DELETE;
    }
    if (!Players.IsInvincible() && !lp->IsDead()) {
      lp->HitCheck();
    }
  }
  reflect.Compact([](const LaserReflect &l) { return l.IsDead(); });

  MoveLong();
  MoveHoming();
}

void LaserManager::DrawAll() const {
  GrpGeom->Lock();

  for (uint16_t i = 0; i < reflect.count; i++) {
    reflect.Active(i).Draw();
  }

  GrpGeom->Unlock();
}

void LaserManager::ClearAll() {
  for (uint16_t i = 0; i < reflect.count; i++) {
    reflect.Active(i).StartClear();
  }
  ClearLong();
  ClearHoming();
}
