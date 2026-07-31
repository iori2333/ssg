///
/// BulletManager — Centralized bullet and laser system state
///

#include <array>
#include <cmath>
#include <cstdlib>
#include <span>

#include "bullet.h"
#include "bullet_common.h"
#include "bullet_manager.h"

#include "audio/snd.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "item/item_system.h"
#include "player/player.h"
#include "util/math_utils.h"

// ── BulletManager: Init ──────────────────────────────────────────

void BulletManager::Init() {
  bullets_.Reset();
  reflect_lasers_.Reset();
  long_lasers_.Reset();
  homing_lasers_.Reset();
  Snd_SEStop(SfxId::Laser);
}

// ── BulletManager: Spawn (bullets) ────────────────────────────────

void BulletManager::SpawnBullet(const BulletSpawnInfo &si) {
  switch (si.spawn_type) {
  case BulletSpawnType::Normal:
    SpawnBulletNormal(si);
    return;
  case BulletSpawnType::Line:
    SpawnBulletLine(si);
    return;
  case BulletSpawnType::Extra01:
    SpawnBulletExtra01(si);
    return;
  }
}

void BulletManager::SpawnBulletNormal(const BulletSpawnInfo &si) {
  const auto n = si.count;
  const uint16_t setmax = n * (si.rapid ? si.rapid_count : 1U);

  auto base_angle = si.aimed
                        ? math::AngleTo(static_cast<float>(player_.X() - si.x),
                                        static_cast<float>(player_.Y() - si.y))
                        : 0.0f;
  base_angle += si.angle;

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = bullets_.Alloc();
    if (t == nullptr) {
      return;
    }
    auto si2 = si;
    si2.angle = bullet_common::CalcSpreadAngle(i % n, si.pattern, n, base_angle,
                                               si.spread);

    float temp = 0.0f;
    switch (si.speed_variance) {
    case BulletSpeedVariance::None:
      break;
    case BulletSpeedVariance::Small:
      temp = (math::RandomInt() % 16) - 8;
      break;
    case BulletSpeedVariance::Medium:
      temp = (math::RandomInt() % 32) - 16;
      break;
    case BulletSpeedVariance::Large:
      temp = (math::RandomInt() % 64) - 32;
      break;
    }
    si2.speed = si.speed + temp;
    if (si.rapid) {
      si2.speed += (si.speed * 0.125f) * (i / n);
    }
    t->Spawn(si2);
  }
}

void BulletManager::SpawnBulletLine(const BulletSpawnInfo &si) {
  const auto n = si.count;
  uint16_t setmax = n * (si.rapid ? si.rapid_count : 1U);

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = bullets_.Alloc();
    if (t == nullptr) {
      return;
    }
    auto si2 = si;
    si2.angle = bullet_common::CalcSpreadAngle(i % n, BulletPattern::Spread, n,
                                               si.angle, si.spread);

    const int i_mod = (i % n) + 1;
    const auto relative_angle = math::ShortestAngleDelta(si2.angle, si.angle);
    float v_ret = si.speed / std::cos(relative_angle);
    if (si.rapid) {
      v_ret += (v_ret * 0.125f) * (i_mod - 1);
    }
    si2.speed = v_ret;

    t->Spawn(si2);
  }
}

void BulletManager::SpawnBulletExtra01(const BulletSpawnInfo &si) {
  const auto n = si.count;
  const uint16_t setmax = n * (si.rapid ? si.rapid_count : 1U);

  for (uint16_t i = 0; i < setmax; i++) {
    auto *t = bullets_.Alloc();
    if (t == nullptr) {
      return;
    }
    auto si2 = si;
    si2.angle = bullet_common::CalcSpreadAngle(i % n, si.pattern, n, si.angle,
                                               si.spread);

    float temp = 0.0f;
    switch (si.speed_variance) {
    case BulletSpeedVariance::None:
      break;
    case BulletSpeedVariance::Small:
      temp = (math::RandomInt() % 16) - 8;
      break;
    case BulletSpeedVariance::Medium:
      temp = (math::RandomInt() % 32) - 16;
      break;
    case BulletSpeedVariance::Large:
      temp = (math::RandomInt() % 64) - 32;
      break;
    }
    const auto delta = std::abs(math::ShortestAngleDelta(si.angle, si2.angle));
    si2.speed =
        si.speed - ((si.speed * delta) / math::AngleFromLegacy(23)) + temp;

    t->Spawn(si2);
  }
}

// ── BulletManager: Spawn (lasers) ─────────────────────────────────

void BulletManager::SpawnReflect(const ReflectSpawnInfo &info) {
  auto cmd = info;
  if (!cmd.no_scaling) {
    switch (session_.EffectiveLevel()) {
    case GameLevel::Easy:
      bullet_common::ApplyEasyCountSpread(cmd.pattern, cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthEasy(cmd.l);
      break;
    case GameLevel::Hard:
    case GameLevel::Extra:
      bullet_common::ApplyHardCountSpread(cmd.pattern, cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthHard(cmd.l);
      break;
    case GameLevel::Lunatic:
      bullet_common::ApplyLunaticCountSpread(cmd.pattern, cmd.n, cmd.dw);
      cmd.l = bullet_common::ScaleLengthLunatic(cmd.l);
      break;
    case GameLevel::Normal:
    default:
      break;
    }
    cmd.v = bullet_common::ScaleVelocityByRank(cmd.v, session_.rank);
  }

  auto base_angle = cmd.aimed
                        ? math::AngleTo(static_cast<float>(player_.X() - cmd.x),
                                        static_cast<float>(player_.Y() - cmd.y))
                        : 0.0f;
  base_angle += cmd.angle;

  for (uint8_t i = 0; i < cmd.n; i++) {
    auto *r = reflect_lasers_.Alloc();
    if (r == nullptr) {
      return;
    }
    auto si = cmd;
    si.base_angle = base_angle;
    si.bullet_index = i;
    r->Spawn(si);
  }
}

bool BulletManager::SpawnLongLaser(const LongLaserSpawnInfo &info) {
  if (info.enemy == nullptr) {
    return false;
  }
  auto si = info;
  si.player_x = player_.X();
  si.player_y = player_.Y();
  auto *lp = long_lasers_.Alloc();
  if (lp == nullptr) {
    return false;
  }
  lp->Spawn(si);
  return true;
}

void BulletManager::SpawnHoming(const HomingSpawnInfo &info) {
  for (int i = 0; i < static_cast<int>(info.n); i++) {
    auto *p = homing_lasers_.Alloc();
    if (p == nullptr) {
      return;
    }
    auto si = info;
    si.bullet_index = i;
    p->Spawn(si);
  }
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
  const BulletUpdateInfo info{player_.X(), player_.Y(), target.active, target.x,
                              target.y};

  for (auto &b : bullets_) {
    auto r = b.Update(info);
    if (r.smoke_spawn) {
      effects_.SpawnFragment(r.smoke_x, r.smoke_y, FragmentKind::Smoke);
    }
    if (r.division_requested) {
      Snd_SEPlay(SfxId::Joint, r.division_cx);
      auto si = r.division_info;
      ScaleBulletSpawnInfo(si, session_);
      SpawnBullet(si);
    }
  }
  bullets_.Compact([](const Bullet &b) { return b.IsDead(); });
}

void BulletManager::UpdateReflect() {
  std::array<const LaserLong *, kLongLaserCapacity> active_longs{};
  std::size_t long_count = 0;
  for (const auto &ll : long_lasers_) {
    active_longs[long_count++] = &ll;
  }
  std::span<const LaserLong *> long_span(active_longs.data(), long_count);

  for (auto &r : reflect_lasers_) {
    auto result = r.Update(ReflectUpdateInfo{long_span});
    if (result.spawn_requested) {
      SpawnReflect(result.spawn_info);
    }
  }
  reflect_lasers_.Compact([](const LaserReflect &l) { return l.IsDead(); });
}

void BulletManager::UpdateLong() {
  for (auto &ll : long_lasers_) {
    ll.Update();
  }
  long_lasers_.Compact([](const LaserLong &l) { return l.IsDead(); });
}

void BulletManager::UpdateHoming() {
  const auto pi = HomingUpdateInfo{player_.X(), player_.Y()};
  for (auto &h : homing_lasers_) {
    h.Update(pi);
  }
  homing_lasers_.Compact([](const LaserHoming &h) { return h.IsDead(); });
}

// ── BulletManager: HitCheck ───────────────────────────────────────

void BulletManager::HitCheck() {
  if (player_.IsInvincible()) {
    return;
  }
  const int px = player_.X();
  const int py = player_.Y();
  const int player_radius = player_.HitRadius();

  for (auto &b : bullets_) {
    switch (b.CheckHit(px, py, player_radius)) {
    case HitResult::Hit:
      b.RemoveImmediately();
      player_.OnHit();
      return;
    case HitResult::Graze:
      if (b.RegisterGraze()) {
        player_.AddEvadeEx(b.X(), b.Y(), kBulletGrazeValue);
      } else {
        player_.AddEvadeEx(b.X(), b.Y(), 0);
      }
      break;
    default:
      break;
    }
  }

  for (auto &laser : reflect_lasers_) {
    switch (laser.CheckHit(px, py, player_radius)) {
    case HitResult::Hit:
      player_.OnHit();
      return;
    case HitResult::Graze:
      player_.AddEvade(laser.RegisterGraze() ? 3 : 0);
      break;
    case HitResult::Miss:
      break;
    }
  }

  const auto check_lasers = [&](const auto &lpool) {
    for (const auto &laser : lpool) {
      switch (laser.CheckHit(px, py, player_radius)) {
      case HitResult::Hit:
        player_.OnHit();
        return true;
      case HitResult::Graze:
        player_.AddEvade(kBulletEvadeValue);
        break;
      case HitResult::Miss:
        break;
      }
    }
    return false;
  };

  if (check_lasers(long_lasers_)) {
    return;
  }
  check_lasers(homing_lasers_);
}

// ── BulletManager: Render ─────────────────────────────────────────

void BulletManager::Render() const {
  GrpGeom->Lock();
  for (const auto &ll : long_lasers_) {
    ll.Render();
  }
  for (const auto &h : homing_lasers_) {
    h.Render();
  }
  for (const auto &r : reflect_lasers_) {
    r.Render();
  }
  GrpGeom->Unlock();

  for (const auto &b : bullets_) {
    b.Render();
  }
}

// ── BulletManager: Clear / Score / Items ──────────────────────────

void BulletManager::Clear() {
  for (auto &b : bullets_) {
    b.Kill();
  }
  bullets_.Compact([](const Bullet &b) { return b.IsDead(); });

  for (auto &r : reflect_lasers_) {
    r.Kill();
  }
  for (auto &ll : long_lasers_) {
    ll.Kill();
  }
  Snd_SEStop(SfxId::Laser);
  for (auto &h : homing_lasers_) {
    h.Kill();
  }
  homing_lasers_.Compact([](const LaserHoming &h) { return h.IsDead(); });
}

uint32_t BulletManager::ConvertBulletsToScore() {
  uint32_t sum = 0;
  uint32_t score = kBulletClearScoreStart + player_.GrazeCount() * 100;
  for (auto &b : bullets_) {
    if (!b.IsSmall()) {
      continue;
    }
    if (!b.IsClearing()) {
      effects_.SpawnPointValue(b.X() - 4_px, b.Y() - 4_px, score);
      sum += score;
      b.RemoveImmediately();
    }
  }
  bullets_.Compact([](const Bullet &b) { return b.IsDead(); });

  score = kBulletClearScoreEnd + player_.GrazeCount() * 100;
  for (auto &b : bullets_) {
    if (b.IsSmall()) {
      continue;
    }
    if (!b.IsClearing()) {
      effects_.SpawnPointValue(b.X() - 8_px, b.Y() - 8_px, score);
      sum += score;
      b.RemoveImmediately();
    }
  }
  bullets_.Compact([](const Bullet &b) { return b.IsDead(); });

  return sum;
}

void BulletManager::ConvertBulletsToItems(uint8_t frequency) {
  if (frequency == 0) {
    Clear();
    return;
  }

  for (auto &b : bullets_) {
    if (!b.IsClearing()) {
      if (math::RandomInt() % frequency == 0) {
        items_.Spawn(b.X(), b.Y(), ItemKind::Score);
        b.RemoveImmediately();
      } else {
        b.Kill();
      }
    }
  }
  bullets_.Compact([](const Bullet &b) { return b.IsDead(); });
}

// ── Laser control ─────────────────────────────────────────────────

void BulletManager::ControlLongLaser(const EnemyActor *e, uint8_t id,
                                     const LongLaserUpdateInfo &info) {
  for (auto &ll : long_lasers_) {
    if (!ll.BelongsTo(e, id)) {
      continue;
    }
    ll.Update(info);
    switch (info.command) {
    case LongLaserUpdateInfo::Command::Open:
      Snd_SEPlay(SfxId::Laser, ll.X(), true);
      break;
    case LongLaserUpdateInfo::Command::Close:
    case LongLaserUpdateInfo::Command::CloseToLine:
    case LongLaserUpdateInfo::Command::ForceClose:
      Snd_SEStop(SfxId::Laser);
      break;
    default:
      break;
    }
  }
}

// ── Gallery / debug helpers ───────────────────────────────────────

void BulletManager::PlaceDisplayBullet(int x, int y, uint8_t color) {
  auto *t = bullets_.Alloc();
  if (t != nullptr) {
    t->Spawn(BulletSpawnInfo{.x = x, .y = y, .visual = color});
  }
}

void BulletManager::RotateDisplayAngles() {
  for (auto &t : bullets_) {
    t.UpdateDisplayAngle();
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

  for (const auto &b : bullets_) {
    b.RenderDebugHitbox(mode);
  }
  for (const auto &r : reflect_lasers_) {
    r.RenderDebugHitbox(mode);
  }
  for (const auto &ll : long_lasers_) {
    ll.RenderDebugHitbox(mode);
  }
  for (const auto &h : homing_lasers_) {
    h.RenderDebugHitbox(mode);
  }
}
