///
/// BulletManager — Centralized bullet and laser system state
///

#include <array>
#include <span>

#include "bullet.h"
#include "bullet_common.h"
#include "bullet_manager.h"

#include "audio/snd.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_system.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "util/ut_math.h"

namespace {
inline constexpr auto kZSetFlag = 0x08;
} // namespace

// ── BulletManager: Init ──────────────────────────────────────────

void BulletManager::Init() {
  pool.Init();
  reflect.Init();
  long_lasers.Init();
  homing.Init();
  Snd_SEStop(2);
}

// ── BulletManager: Spawn (bullets) ────────────────────────────────

bool BulletManager::SpawnBullet(const BulletSpawnInfo &si) {
  switch (si.spawn_type) {
  case BulletSpawnType::Normal:
    return SpawnBulletNormal(si);
  case BulletSpawnType::Line:
    return SpawnBulletLine(si);
  case BulletSpawnType::Extra01:
    return SpawnBulletExtra01(si);
  }
  return false;
}

bool BulletManager::SpawnBulletNormal(const BulletSpawnInfo &si) {
  const auto n = si.n;
  const uint16_t setmax = n * (si.rapid ? si.ns : 1U);

  auto base_deg =
      si.zset ? atan8(player_->X() - si.x, player_->Y() - si.y) : uint8_t{0};
  base_deg = static_cast<uint8_t>(base_deg + si.d);

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = pool.Alloc();
    if (t == nullptr) {
      return false;
    }
    auto si2 = si;
    si2.d =
        bullet_common::CalcSpreadDir(i % n, si.cmd_type, n, base_deg, si.dw);

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
  return true;
}

bool BulletManager::SpawnBulletLine(const BulletSpawnInfo &si) {
  const auto n = si.n;
  uint16_t setmax = n * (si.rapid ? si.ns : 1U);

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = pool.Alloc();
    if (t == nullptr) {
      return false;
    }
    auto si2 = si;
    si2.d = bullet_common::CalcSpreadDir(i % n, TC_WAY, n, si.d, si.dw);

    const int i_mod = (i % n) + 1;
    const auto deg_factor = ((i_mod >> 1) * si.dw * (1 - ((i_mod & 1) << 1)));
    const uint8_t deg =
        ((n & 1) != 0) ? deg_factor : -(si.dw >> 1) + deg_factor;
    int v_ret = cosDiv(deg, si.v_);
    if (si.rapid) {
      v_ret += (v_ret >> 3) * (i_mod - 1);
    }
    si2.v_ = v_ret;

    t->Spawn(si2);
  }
  return true;
}

bool BulletManager::SpawnBulletExtra01(const BulletSpawnInfo &si) {
  const auto n = si.n;
  const uint16_t setmax = n * (si.rapid ? si.ns : 1U);

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
}

// ── BulletManager: Spawn (lasers) ─────────────────────────────────

bool BulletManager::SpawnReflect(const ReflectSpawnInfo &info) {
  auto cmd = info;
  if (!cmd.no_scaling) {
    switch (game_->EffectiveLevel()) {
    case GameLevel::EASY:
      bullet_common::ApplyEasyCountSpread(cmd.cmd & bullet_common::kCmdMask,
                                          cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthEasy(cmd.l);
      break;
    case GameLevel::HARD:
    case GameLevel::EXTRA:
      bullet_common::ApplyHardCountSpread(cmd.cmd & bullet_common::kCmdMask,
                                          cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthHard(cmd.l);
      break;
    case GameLevel::LUNATIC:
      bullet_common::ApplyLunaticCountSpread(cmd.cmd & bullet_common::kCmdMask,
                                             cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthLunatic(cmd.l);
      break;
    case GameLevel::NORMAL:
    default:
      break;
    }
    cmd.v = bullet_common::ScaleVelocityByRank(cmd.v, game_->rank);
  }

  auto base_deg = (cmd.cmd & kZSetFlag) != 0
                      ? atan8(player_->X() - cmd.x, player_->Y() - cmd.y)
                      : 0;
  base_deg += cmd.d;

  for (uint8_t i = 0; i < cmd.n; i++) {
    auto *r = reflect.Alloc();
    if (r == nullptr) {
      return false;
    }
    auto si = cmd;
    si.base_deg = base_deg;
    si.bullet_index = i;
    r->Spawn(si);
  }
  return true;
}

bool BulletManager::SpawnLongLaser(const LongLaserSpawnInfo &info) {
  if (info.enemy == nullptr) {
    return false;
  }
  auto si = info;
  si.player_x = player_->X();
  si.player_y = player_->Y();
  auto *lp = long_lasers.Alloc();
  if (lp == nullptr) {
    return false;
  }
  lp->Spawn(si);
  return true;
}

bool BulletManager::SpawnHoming(const HomingSpawnInfo &info) {
  for (int i = 1; i <= static_cast<int>(info.n); i++) {
    auto *p = homing.Alloc();
    if (p == nullptr) {
      return false;
    }
    auto si = info;
    si.bullet_index = i;
    p->Spawn(si);
  }
  return true;
}

// ── BulletManager: Update ─────────────────────────────────────────

void BulletManager::Update(const EnemyHomingTarget &target) {
  UpdateBullet(target);
  UpdateReflect();
  UpdateLong();
  UpdateHoming();

  HitCheck();
}

void BulletManager::UpdateBullet(const EnemyHomingTarget &target) {
  const BulletUpdateInfo info{player_->X(), player_->Y(), target.active,
                              target.x, target.y};

  for (auto &b : pool) {
    auto r = b.Update(info);
    if (r.smoke_spawn) {
      Effects.SpawnFragment(r.smoke_x, r.smoke_y, FRG_SMOKE);
    }
    if (r.division_requested) {
      Snd_SEPlay(static_cast<SfxId>(12), r.division_cx);
      auto si = MakeBulletSpawnInfo(r.division_cmd, 0, 0, true, *game_);
      SpawnBullet(si);
    }
  }
  pool.Compact([](const Bullet &b) { return b.IsDead(); });
}

void BulletManager::UpdateReflect() {
  std::array<const LaserLong *, kLongLaserMax> active_longs{};
  std::size_t long_count = 0;
  for (const auto &ll : long_lasers) {
    active_longs[long_count++] = &ll;
  }
  std::span<const LaserLong *> long_span(active_longs.data(), long_count);

  for (auto &r : reflect) {
    auto result = r.Update(ReflectUpdateInfo{long_span});
    if (result.spawn_requested) {
      SpawnReflect(result.spawn_info);
    }
    if (r.X() < GX_MIN || r.X() > GX_MAX || r.Y() < GY_MIN || r.Y() > GY_MAX) {
      r.MarkDead();
    }
  }
  reflect.Compact([](const LaserReflect &l) { return l.IsDead(); });
}

void BulletManager::UpdateLong() {
  for (auto &ll : long_lasers) {
    ll.Update();
  }
  long_lasers.Compact([](const LaserLong &l) { return l.IsDead(); });
}

void BulletManager::UpdateHoming() {
  const auto pi = HomingUpdateInfo{player_->X(), player_->Y()};
  for (auto &h : homing) {
    h.Update(pi);
  }
  homing.Compact([](const LaserHoming &h) { return h.IsDead(); });
}

// ── BulletManager: HitCheck ───────────────────────────────────────

void BulletManager::HitCheck() {
  if (player_->IsInvincible()) {
    return;
  }
  const int px = player_->X();
  const int py = player_->Y();

  for (auto &b : pool) {
    switch (b.CheckHit(px, py)) {
    case HitResult::Hit:
      b.MarkDead();
      player_->OnHit();
      return;
    case HitResult::Graze:
      if (!b.HasGrazed()) {
        b.MarkGrazed();
        player_->AddEvadeEx(b.X(), b.Y(), TAMA_EVADE);
      } else {
        player_->AddEvadeEx(b.X(), b.Y(), 0);
      }
      break;
    default:
      break;
    }
  }

  auto check_lasers = [&](const auto &lpool) {
    for (const auto &laser : lpool) {
      switch (laser.CheckHit(px, py)) {
      case HitResult::Hit:
        player_->OnHit();
        return;
      case HitResult::Graze:
        player_->AddEvade(kBulletEvadeValue);
        break;
      case HitResult::Miss:
        break;
      }
    }
  };

  check_lasers(reflect);
  check_lasers(long_lasers);
  check_lasers(homing);
}

// ── BulletManager: Render ─────────────────────────────────────────

void BulletManager::Render() const {
  GrpGeom->Lock();
  for (const auto &ll : long_lasers) {
    ll.Render();
  }
  for (const auto &h : homing) {
    h.Render();
  }
  for (const auto &r : reflect) {
    r.Render();
  }
  GrpGeom->Unlock();

  for (const auto &b : pool) {
    b.Render();
  }
}

// ── BulletManager: Clear / Score / Items ──────────────────────────

void BulletManager::Clear() {
  for (auto &b : pool) {
    b.Kill();
  }
  pool.Compact([](const Bullet &b) { return b.IsDead(); });

  for (auto &r : reflect) {
    r.Kill();
  }
  for (auto &ll : long_lasers) {
    ll.Kill();
  }
  Snd_SEStop(2);
  for (auto &h : homing) {
    h.Kill();
  }
  homing.Compact([](const LaserHoming &h) { return h.IsDead(); });
}

uint32_t BulletManager::ScoreToItems() {
  uint32_t sum = 0;
  uint32_t score = TAMA1_POINT + player_->GrazeCount() * 100;
  for (auto &b : pool) {
    if ((b.c_ & 0xf0) != TAMA_SMALL) {
      continue;
    }
    if (b.effect_ != TE_DELETE) {
      Effects.SpawnPointEffect(b.x_ - 64 * 4, b.y_ - 64 * 4, score);
      sum += score;
      b.flag_ = TF_DELETE;
      b.count_ = 0;
      b.c_ = 0x25;
    }
  }
  pool.Compact([](const Bullet &b) { return b.IsDead(); });

  score = TAMA2_POINT + player_->GrazeCount() * 100;
  for (auto &b : pool) {
    if ((b.c_ & 0xf0) == TAMA_SMALL) {
      continue;
    }
    if (b.effect_ != TE_DELETE) {
      Effects.SpawnPointEffect(b.x_ - 64 * 8, b.y_ - 64 * 8, score);
      sum += score;
      b.flag_ = TF_DELETE;
      b.count_ = 0;
      b.c_ = 0x25;
    }
  }
  pool.Compact([](const Bullet &b) { return b.IsDead(); });

  return sum;
}

void BulletManager::ToItems(uint8_t n) {
  if (n == 0) {
    Clear();
    return;
  }

  for (auto &b : pool) {
    if (b.effect_ != TE_DELETE) {
      b.count_ = 0;
      if (rnd() % n == 0) {
        items_->Spawn(b.x_, b.y_, ITEM_SCORE);
        b.flag_ = TF_DELETE;
        b.c_ = 0x25;
      } else {
        b.effect_ = TE_DELETE;
        b.count_ = 0;
      }
    }
  }
  pool.Compact([](const Bullet &b) { return b.IsDead(); });
}

// ── Laser control ─────────────────────────────────────────────────

void BulletManager::ControlLongLaser(const EnemyActor *e, uint8_t id,
                                     const LongLaserUpdateInfo &info) {
  for (auto &ll : long_lasers) {
    if (!ll.BelongsTo(e, id)) {
      continue;
    }
    ll.Update(info);
    switch (info.command) {
    case LongLaserUpdateInfo::Command::Open:
      Snd_SEPlay(static_cast<SfxId>(2), ll.X(), true);
      break;
    case LongLaserUpdateInfo::Command::Close:
    case LongLaserUpdateInfo::Command::CloseToLine:
    case LongLaserUpdateInfo::Command::ForceClose:
      Snd_SEStop(2);
      break;
    default:
      break;
    }
  }
}

// ── Gallery / debug helpers ───────────────────────────────────────

void BulletManager::PlaceDisplayBullet(int x, int y, uint8_t color) {
  auto *t = pool.Alloc();
  if (t != nullptr) {
    t->x_ = x;
    t->y_ = y;
    t->c_ = color;
  }
}

void BulletManager::RotateDisplayAngles() {
  for (auto &t : pool) {
    const auto cat = t.c_ & 0xF0;
    if (cat == TAMA_ANGLE || cat == TAMA_EXTRA2) {
      t.d_ += 4;
    }
  }
}

void BulletManager::RenderDebugHitboxes(int mode) const {
  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  const RGB216 kBlack{0, 0, 0};
  constexpr uint8_t kAlpha = 204;
  gp->SetColor(kBlack);
  gp->SetAlphaNorm(kAlpha);

  for (const auto &b : pool) {
    b.RenderDebugHitbox(mode);
  }
  for (const auto &r : reflect) {
    r.RenderDebugHitbox(mode);
  }
  for (const auto &ll : long_lasers) {
    ll.RenderDebugHitbox(mode);
  }
  for (const auto &h : homing) {
    h.RenderDebugHitbox(mode);
  }
}
