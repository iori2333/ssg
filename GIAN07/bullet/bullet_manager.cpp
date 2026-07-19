///
/// BulletManager — Centralized bullet system state
///

#include "bullet.h"
#include "bullet_common.h"
#include "bullet_manager.h"

#include "audio/snd.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "item/item_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "gameflow/rank_manager.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "player/player.h"
#include "util/ut_math.h"

// ── Global instance ──────────────────────────────────────────────

BulletManager Bullets;

// ── BulletManager: Init ──────────────────────────────────────────

void BulletManager::Init() {
  pool_small.Init();
  pool_large.Init();
}

// ── BulletManager: Spawn ─────────────────────────────────────────

template <typename Pool>
void BulletManager::SpawnImpl(const BulletSpawnInfo &si, Pool &pool) {
  const auto n = si.n;
  const uint16_t setmax = n * (si.rapid ? si.ns : 1U);

  auto base_deg =
      si.zset ? atan8(Players.X() - si.x, Players.Y() - si.y) : uint8_t{0};
  base_deg = static_cast<uint8_t>(base_deg + si.d);

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = pool.Alloc();
    if (t == nullptr) {
      return;
    }
    auto si2 = si;
    si2.d = bullet_common::CalcSpreadDir(i % n, si.cmd_type, n, base_deg, si.dw);

    int temp = 0;
    switch (si.vsp) {
    case TAMASP_RND1:
      temp = (rnd() % 16) - 8;
      break;
    case TAMASP_RND2:
      temp = (rnd() % 32) - 16;
      break;
    case TAMASP_RND3:
      temp = (rnd() % 64) - 32;
      break;
    }
    si2.v_ = si.v_ + temp;
    if (si.rapid) {
      si2.v_ += (si.v_ >> 3) * (i / n);
    }
    t->Spawn(si2);
  }
}

bool BulletManager::Spawn(const BulletSpawnInfo &si) {
  if ((si.c & 0xf0) == TAMA_SMALL) {
    SpawnImpl(si, pool_small);
  } else {
    SpawnImpl(si, pool_large);
  }
  return true;
}

bool BulletManager::SpawnLine(const BulletSpawnInfo &si) {
  auto si2 = si;
  si2.cmd_type = TC_WAY;
  si2.rapid = false;

  const auto n = si.n;
  const auto rapid = (si.dw != 0) ? false : false; // line uses si.rapid logic
  uint16_t setmax = n * (si.rapid ? si.ns : 1U);

  auto do_spawn = [&](auto &pool) -> bool {
    for (uint16_t i = 0; i < setmax; i++) {
      auto *t = pool.Alloc();
      if (t == nullptr) {
        return false;
      }
      auto si3 = si2;
      si3.d = bullet_common::CalcSpreadDir(i % n, TC_WAY, n, si.d, si.dw);

      const int i_mod = (i % n) + 1;
      const auto deg_factor =
          ((i_mod >> 1) * si.dw * (1 - ((i_mod & 1) << 1)));
      const uint8_t deg =
          ((n & 1) != 0) ? deg_factor : -(si.dw >> 1) + deg_factor;
      int v_ret = cosDiv(deg, si.v_);
      if (si.rapid) {
        v_ret += (v_ret >> 3) * (i_mod - 1);
      }
      si3.v_ = v_ret;

      t->Spawn(si3);
    }
    return true;
  };

  if ((si.c & 0xf0) == TAMA_SMALL) {
    return do_spawn(pool_small);
  }
  return do_spawn(pool_large);
}

bool BulletManager::SpawnExtra01(const BulletSpawnInfo &si) {
  const auto n = si.n;
  const uint16_t setmax = n * (si.rapid ? si.ns : 1U);

  auto do_spawn = [&](auto &pool) -> bool {
    for (uint16_t i = 0; i < setmax; i++) {
      auto *t = pool.Alloc();
      if (t == nullptr) {
        return false;
      }
      auto si2 = si;
      si2.d = bullet_common::CalcSpreadDir(i % n, si.cmd_type, n, si.d, si.dw);

      int temp = 0;
      switch (si.vsp) {
      case TAMASP_RND1:
        temp = (rnd() % 16) - 8;
        break;
      case TAMASP_RND2:
        temp = (rnd() % 32) - 16;
        break;
      case TAMASP_RND3:
        temp = (rnd() % 64) - 32;
        break;
      }
      int delta = static_cast<int>(si.d) - static_cast<int>(si2.d);
      if (delta > 128) {
        delta -= 256;
      }
      if (delta < -128) {
        delta += 256;
      }
      si2.v_ = si.v_ - ((si.v_ * abs(delta)) / 23) + temp;

      t->Spawn(si2);
    }
    return true;
  };

  if ((si.c & 0xf0) == TAMA_SMALL) {
    return do_spawn(pool_small);
  }
  return do_spawn(pool_large);
}

// ── BulletManager: Update & HitCheck ─────────────────────────────

void BulletManager::UpdateAll() {
  const BulletUpdateInfo info{Players.X(), Players.Y(),
                              Enemies.homing_flag != HOMING_DUMMY,
                              Enemies.homing_x, Enemies.homing_y};

  for (auto &b : pool_small) {
    auto r = b.Update(info);
    if (r.smoke_spawn) {
      Effects.SpawnFragment(r.smoke_x, r.smoke_y, FRG_SMOKE);
    }
    if (r.division_requested) {
      Snd_SEPlay(static_cast<SfxId>(12), r.division_cx);
      auto si = MakeBulletSpawnInfo(r.division_cmd, 0, 0, true);
      Spawn(si);
    }
  }
  pool_small.Compact([](const Bullet &b) { return b.IsDead(); });

  for (auto &b : pool_large) {
    auto r = b.Update(info);
    if (r.smoke_spawn) {
      Effects.SpawnFragment(r.smoke_x, r.smoke_y, FRG_SMOKE);
    }
    if (r.division_requested) {
      Snd_SEPlay(static_cast<SfxId>(12), r.division_cx);
      auto si = MakeBulletSpawnInfo(r.division_cmd, 0, 0, true);
      Spawn(si);
    }
  }
  pool_large.Compact([](const Bullet &b) { return b.IsDead(); });

  HitCheckAll();
}

void BulletManager::HitCheckAll() {
  if (Players.IsInvincible() != 0U) {
    return;
  }
  const int px = Players.X();
  const int py = Players.Y();

  auto check = [&](auto &pool) {
    for (auto &b : pool) {
      switch (b.CheckHit(px, py)) {
      case HitResult::Hit:
        b.MarkDead();
        Players.OnHit();
        return;
      case HitResult::Graze:
        if (!b.HasGrazed()) {
          b.MarkGrazed();
          Players.AddEvadeEx(b.X(), b.Y(), TAMA_EVADE);
        } else {
          Players.AddEvadeEx(b.X(), b.Y(), 0);
        }
        break;
      default:
        break;
      }
    }
  };
  check(pool_small);
  check(pool_large);
}

// ── BulletManager: Render ────────────────────────────────────────

void BulletManager::RenderAll() const {
  for (const auto &b : pool_large) {
    b.Render();
  }
  for (const auto &b : pool_small) {
    b.Render();
  }
}

// ── BulletManager: Clear / Score / Items ─────────────────────────

void BulletManager::ClearAll() {
  for (auto &b : pool_small) {
    b.Kill();
  }
  for (auto &b : pool_large) {
    b.Kill();
  }
  pool_small.Compact([](const Bullet &b) { return b.IsDead(); });
  pool_large.Compact([](const Bullet &b) { return b.IsDead(); });
}

uint32_t BulletManager::ScoreToItems() {
  uint32_t sum = 0;
  uint32_t score = TAMA1_POINT + Players.GrazeCount() * 100;
  for (auto &b : pool_small) {
    if (b.effect_ != TE_DELETE) {
      Effects.SpawnPointEffect(b.x_ - 64 * 4, b.y_ - 64 * 4, score);
      sum += score;
      b.flag_ = TF_DELETE;
      b.count_ = 0;
      b.c_ = 0x25;
    }
  }
  pool_small.Compact([](const Bullet &b) { return b.IsDead(); });

  score = TAMA2_POINT + Players.GrazeCount() * 100;
  for (auto &b : pool_large) {
    if (b.effect_ != TE_DELETE) {
      Effects.SpawnPointEffect(b.x_ - 64 * 8, b.y_ - 64 * 8, score);
      sum += score;
      b.flag_ = TF_DELETE;
      b.count_ = 0;
      b.c_ = 0x25;
    }
  }
  pool_large.Compact([](const Bullet &b) { return b.IsDead(); });

  return sum;
}

void BulletManager::ToItems(uint8_t n) {
  if (n == 0) {
    ClearAll();
    return;
  }

  for (auto &b : pool_small) {
    if (b.effect_ != TE_DELETE) {
      b.count_ = 0;
      if (rnd() % n == 0) {
        Items.Spawn(b.x_, b.y_, ITEM_SCORE);
        b.flag_ = TF_DELETE;
        b.c_ = 0x25;
      } else {
        b.effect_ = TE_DELETE;
        b.count_ = 0;
      }
    }
  }
  pool_small.Compact([](const Bullet &b) { return b.IsDead(); });

  for (auto &b : pool_large) {
    if (b.effect_ != TE_DELETE) {
      b.count_ = 0;
      if (rnd() % n == 0) {
        Items.Spawn(b.x_, b.y_, ITEM_SCORE);
        b.flag_ = TF_DELETE;
        b.c_ = 0x25;
      } else {
        b.effect_ = TE_DELETE;
        b.count_ = 0;
      }
    }
  }
  pool_large.Compact([](const Bullet &b) { return b.IsDead(); });
}

// ── Gallery / debug helpers ─────────────────────────────────────

void BulletManager::PlaceDisplayBullet(int x, int y, uint8_t color) {
  if ((color & 0xF0) == TAMA_SMALL) {
    auto *t = pool_small.Alloc();
    if (t != nullptr) {
      t->x_ = x;
      t->y_ = y;
      t->c_ = color;
    }
  } else {
    auto *t = pool_large.Alloc();
    if (t != nullptr) {
      t->x_ = x;
      t->y_ = y;
      t->c_ = color;
    }
  }
}

void BulletManager::RotateDisplayAngles() {
  ApplySmall([](Bullet &t) {
    if ((t.c_ & 0xF0) == TAMA_ANGLE) {
      t.d_ += 4;
    }
  });
  ApplyLarge([](Bullet &t) {
    const auto cat = t.c_ & 0xF0;
    if (cat == TAMA_ANGLE || cat == TAMA_EXTRA2) {
      t.d_ += 4;
    }
  });
}

// ── Player shot compat (TODO: remove after player shot refactor) ──

uint8_t BulletManager::Dir(uint16_t i) const {
  uint8_t deg = ((command.cmd & TAMA_ZSET) != 0)
                    ? atan8(Players.X() - command.x, Players.Y() - command.y)
                    : 0;
  deg += command.d;
  i = i % command.n;
  switch (command.cmd & 0x03) {
  case TC_WAY:
    i++;
    if ((command.n & 1) != 0) {
      return deg + ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));
    }
    return deg - (command.dw >> 1) +
           ((i >> 1) * command.dw * (1 - ((i & 1) << 1)));
  case TC_ALL:
    return deg + ((i << 8) / command.n);
  case TC_RND:
    return deg + (rnd() % command.dw) - (command.dw >> 1);
  default:
    return 0;
  }
}

int BulletManager::Speed(uint16_t i) const {
  int temp = 0;
  const int vret = SPEEDM(command.v);
  switch (command.v & 0xc0) {
  case TAMASP_RND1:
    temp = (rnd() % 16) - 8;
    break;
  case TAMASP_RND2:
    temp = (rnd() % 32) - 16;
    break;
  case TAMASP_RND3:
    temp = (rnd() % 64) - 32;
    break;
  }
  if ((command.cmd & TAMA_REN) != 0) {
    return vret + ((vret >> 3) * (i / command.n)) + temp;
  }
  return vret + temp;
}

uint8_t BulletManager::Flag() const {
  switch (command.type) {
  case T_HOMING:
  case T_HOMING_M:
  case T_ROLL:
  case T_ROLL_A:
  case T_ROLL_R:
  case T_SBHOMING:
    return TF_CLIP;
  default:
    return TF_NONE;
  }
}

void BulletManager::MoveByOption(Bullet *t) {
  BulletUpdateInfo::UpdateResult dummy;
  t->MoveByOption(dummy);
}

void BulletManager::MoveByType(Bullet *t) {
  const BulletUpdateInfo info{};
  BulletUpdateInfo::UpdateResult dummy;
  t->MoveByType(info, dummy);
}

void BulletManager::MoveByEffect(Bullet *t) { t->MoveByEffect(); }

// ── Debug ─────────────────────────────────────────────────────────

void BulletManager::RenderDebugHitboxes(int mode) const {
  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  const RGB216 kBlack{0, 0, 0};
  constexpr uint8_t kAlpha = 204;
  gp->SetColor(kBlack);
  gp->SetAlphaNorm(kAlpha);

  for (const auto &b : pool_small) {
    b.RenderDebugHitbox(mode);
  }
  for (const auto &b : pool_large) {
    b.RenderDebugHitbox(mode);
  }
}
