///
/// LaserManager — Centralized laser system state
///

#include <array>
#include <span>

#include "laser_manager.h"

#include "audio/snd.h"
#include "bullet_common.h"
#include "core/gian.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "gameflow/play_rank.h"
#include "gameflow/rank_manager.h"
#include "player/player.h"
#include "util/ut_math.h"

// ── Global instance ─────────────────────────────────────────────────
LaserManager Lasers;

namespace {
inline constexpr auto kZSetFlag = 0x08;
} // namespace

// ── Init ───────────────────────────────────────────────────────────

void LaserManager::Init() {
  reflect.Init();
  long_lasers.Init();
  homing.Init();
  Snd_SEStop(2);
}

// ── Spawn: Reflect lasers ──────────────────────────────────────────

bool LaserManager::SpawnReflect(const ReflectSpawnInfo &info) {
  auto cmd = info;
  if (!cmd.no_scaling) {
    switch (Ranking.state.level) {
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
    cmd.v = bullet_common::ScaleVelocityByRank(cmd.v, Ranking.state.Rank);
  }

  auto base_deg = (cmd.cmd & kZSetFlag) != 0
                      ? atan8(Players.X() - cmd.x, Players.Y() - cmd.y)
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

// ── Spawn: Long lasers ─────────────────────────────────────────────

bool LaserManager::SpawnLongLaser(const LongLaserSpawnInfo &info) {
  auto *lp = long_lasers.Alloc();
  if (lp == nullptr) {
    return false;
  }
  lp->Spawn(info);
  return true;
}

// ── Spawn: Homing lasers ───────────────────────────────────────────

bool LaserManager::SpawnHoming(const HomingSpawnInfo &info) {
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

// ── Per-frame update ───────────────────────────────────────────────

void LaserManager::UpdateAll() {
  UpdateReflect();
  UpdateLong();
  UpdateHoming();
  HitCheckAll();
}

void LaserManager::UpdateReflect() {
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
    if (r.X() < GX_MIN || r.X() > GX_MAX || r.Y() < GY_MIN ||
        r.Y() > GY_MAX) {
      r.MarkDead();
    }
  }
  reflect.Compact([](const LaserReflect &l) { return l.IsDead(); });
}

void LaserManager::UpdateLong() {
  for (auto &ll : long_lasers) {
    ll.Update();
  }
  long_lasers.Compact([](const LaserLong &l) { return l.IsDead(); });
}

void LaserManager::UpdateHoming() {
  for (auto &h : homing) {
    h.Update();
  }
  homing.Compact([](const LaserHoming &h) { return h.IsDead(); });
}

// ── Per-frame collision ───────────────────────────────────────────

void LaserManager::HitCheckAll() const {
  if (Players.IsInvincible()) {
    return;
  }

  const int px = Players.X();
  const int py = Players.Y();

  auto check = [&](auto &pool) {
    for (const auto &laser : pool) {
      switch (laser.CheckHit(px, py)) {
      case HitResult::Hit:
        Players.OnHit();
        return;
      case HitResult::Graze:
        Players.AddEvade(kBulletEvadeValue);
        break;
      case HitResult::Miss:
        break;
      }
    }
  };

  check(reflect);
  check(long_lasers);
  check(homing);
}

// ── Per-frame render ───────────────────────────────────────────────

void LaserManager::RenderAll() const { RenderReflect(); }

void LaserManager::RenderReflect() const {
  GrpGeom->Lock();
  for (const auto &r : reflect) {
    r.Render();
  }
  GrpGeom->Unlock();
}

void LaserManager::RenderLong() const {
  GrpGeom->Lock();
  for (const auto &ll : long_lasers) {
    ll.Render();
  }
  GrpGeom->Unlock();
}

void LaserManager::RenderHoming() const {
  GrpGeom->Lock();
  for (const auto &h : homing) {
    h.Render();
  }
  GrpGeom->Unlock();
}

// ── Clear ──────────────────────────────────────────────────────────

void LaserManager::ClearAll() {
  ClearReflect();
  ClearLong();
  ClearHoming();
}

void LaserManager::ClearReflect() {
  for (auto &r : reflect) {
    r.Kill();
  }
}

void LaserManager::ClearLong() {
  for (auto &ll : long_lasers) {
    ll.Kill();
  }
  Snd_SEStop(2);
}

void LaserManager::ClearHoming() { homing.Init(); }

// ── External control ──────────────────────────────────────────────

void LaserManager::ControlLongLaser(const EnemyData *e, uint8_t id,
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

// ── Debug ──────────────────────────────────────────────────────────

void LaserManager::RenderDebugHitboxes(int mode) const {
  auto *gp = GrpGeom_Poly();
  if (gp == nullptr) {
    return;
  }
  const RGB216 kBlack{0, 0, 0};
  constexpr uint8_t kAlpha = 204;
  gp->SetColor(kBlack);
  gp->SetAlphaNorm(kAlpha);

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
