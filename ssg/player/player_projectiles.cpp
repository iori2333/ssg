///
/// Player projectiles - movement, collision, and drawing.
///
/// Loadout-specific firing lives in player/loadout. This file owns the shared
/// projectile pool and per-frame dispatch.
///

#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

#include "loadout/player_loadout.h"
#include "player.h"
#include "player_attack.h"
#include "player_shot.h"

#include "effect/effect_manager.h"
#include "effect/effect_types.h"
#include "enemy/enemy_manager.h"
#include "gameplay/playfield.h"
#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/world_math.h"
#include "gfx/graphics.h"
#include "stage/stage_session.h"
#include "sys/input.h"
#include "util/math_utils.h"

namespace {
constexpr std::array<uint8_t, std::to_underlying(PlayerShotKind::Count)>
    kShotDamage{
        6, // WideMain
        4, // WideSub
        6, // HomingMain
        7, // HomingSub
        5, // LaserSub
        1, // HomingBomb
        1, // HomingBombBlast
        6, // WideFocusMain
        4, // WideFocusSub
        6, // HomingFocusMain
        7, // HomingFocusSub
    };

constexpr int ShotDamage(PlayerShotKind kind) {
  return kShotDamage[std::to_underlying(kind)];
}
} // namespace

// --- Fire dispatch ---

void Player::UpdateWeapons(EnemyManager &enemies, InputBits input) {
  if (((input & KeyTama) != 0) && toge_time_ == 0 && !IsMovementDisabled()) {
    toge_time_ = kShotCycleStart;
  }

  if ((input & KeyBomb) != 0) {
    const auto trigger = life_state_ == LifeState::DeathbombWindow
                             ? BombTrigger::Deathbomb
                             : BombTrigger::Manual;
    ActivateBomb(trigger);
  }

  if (bomb_time_ != 0U) {
    bomb_time_--;
    loadout_->UpdateBomb(*this, enemies, effects_, bomb_time_);
  }

  if (toge_time_ != 0U) {
    const int tier = (exp_ + 1) >> 5;
    if (IsMainShotFrame(toge_time_)) {
      loadout_->FireMain(*this, tier, focused_);
    }
    if (IsSubShotFrame(toge_time_)) {
      loadout_->FireSub(*this, tier, focused_);
    }
    toge_time_--;
  }

  loadout_->Tick(*this);
}

// --- Movement helpers ---

bool PlayerShot::Move(const EnemyHomingTarget &target) {
  float angle_delta = 0.0F;

  switch (motion_) {
  case PlayerShotMotion::Straight:
    x_ += velocity_x_;
    y_ += velocity_y_;
    return false;

  case PlayerShotMotion::Homing: {
    const bool spawn_smoke = (age_ & 1) != 0;
    const WorldCoord previous_x = x_;
    const WorldCoord previous_y = y_;
    x_ += velocity_x_;
    y_ += velocity_y_;
    if (age_ < 70 && target.active) {
      const auto target_angle = math::AngleTo(
          WorldPoint{target.x - previous_x, target.y - previous_y});
      angle_delta = math::ShortestAngleDelta(target_angle, direction_);
    } else if (age_ < 70) {
      const auto target_angle =
          math::AngleTo(WorldPoint{WorldCoord{}, -20_px - previous_y});
      angle_delta = math::ShortestAngleDelta(target_angle, direction_);
    } else {
      angle_delta = 0.0F;
    }
    if (std::abs(angle_delta) < math::kLegacyAngleStep * 0.5F) {
      if (turn_rate_ != 0) {
        turn_rate_--;
      }
      speed_ += acceleration_;
    } else {
      if (turn_rate_ < 127) {
        turn_rate_++;
      }
      speed_ -= acceleration_;
    }
    direction_ += angle_delta * static_cast<float>(turn_rate_) / 255.0F;
    const auto velocity = math::RoundedPolarVector(direction_, speed_);
    velocity_x_ = velocity.x;
    velocity_y_ = velocity.y;
    return spawn_smoke;
  }

  case PlayerShotMotion::Stationary:
    return false;
  }
  return false;
}

// --- Bullet movement & hit check ---

void Player::UpdateProjectiles(EnemyManager &enemies) {
  for (auto &t : maid_tama_) {
    if (t.kind_ == PlayerShotKind::HomingBombBlast) {
      enemies.ApplyPlayerAttack(
          PlayerAttack::Point(WorldPoint{t.x_, t.y_}, ShotDamage(t.kind_)));
      t.age_++;
      if (t.age_ >= 19) {
        t.pending_removal_ = true;
      }
      continue;
    }

    if (t.Move(enemies.HomingTarget())) {
      effects_.SpawnFragment(t.x_, t.y_, FragmentKind::Smoke);
    }
    t.age_++;
    if (t.x_ < playfield::kWorldLeft || t.x_ > playfield::kWorldRight ||
        t.y_ < playfield::kWorldTop || t.y_ > playfield::kWorldBottom) {
      t.pending_removal_ = true;
    }

    if (enemies.ApplyPlayerAttack(
            PlayerAttack::Point(WorldPoint{t.x_, t.y_}, ShotDamage(t.kind_)))) {
      if (t.kind_ == PlayerShotKind::HomingBomb) {
        PlayerShotSpawnInfo const si{
            .x = t.x_,
            .y = t.y_,
            .direction = 192,
            .direction_step = 16,
            .count = 1,
            .speed = 2.5_px,
            .acceleration = {},
            .kind = PlayerShotKind::HomingBombBlast,
            .motion = PlayerShotMotion::Stationary,
        };
        SpawnShot(si);
      }
      t.pending_removal_ = true;
      effects_.SpawnFragment(t.x_, t.y_, FragmentKind::Hit);
    }
  }
  maid_tama_.Compact([](const PlayerShot &t) { return t.pending_removal_; });

  loadout_->ApplyContinuousAttack(*this, enemies, focused_);
}

// --- Bullet drawing ---

void Player::DrawProjectiles() const {
  int x = 0;
  int y = 0;
  Rect src;
  static constexpr std::array<Rect, 5> HomingBomb = {
      Rect{520, 104, 520 + 8, 104 + 8}, Rect{528, 104, 528 + 16, 104 + 16},
      Rect{544, 104, 544 + 24, 104 + 24}, Rect{568, 104, 568 + 32, 104 + 32},
      Rect{600, 104, 600 + 40, 104 + 40}};

  for (const auto &t : maid_tama_) {

    x = t.x_.ToPixels() - 8;
    y = t.y_.ToPixels() - 8;
    const auto display_angle = math::AngleToLegacy(t.direction_);

    switch (t.kind_) {
    case PlayerShotKind::WideMain:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 176, 16, 16);
      break;
    case PlayerShotKind::WideSub:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 192, 16, 16);
      break;
    case PlayerShotKind::HomingMain:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 208, 16, 16);
      break;
    case PlayerShotKind::HomingSub:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 224, 16, 16);
      break;
    case PlayerShotKind::HomingBomb:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 288, 16, 16);
      break;
    case PlayerShotKind::LaserSub:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 256, 16, 16);
      break;
    case PlayerShotKind::HomingBombBlast:
      src = HomingBomb[(static_cast<int>(t.age_) / 4) % 5];
      break;
    case PlayerShotKind::WideFocusMain:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 176, 16, 16);
      break;
    case PlayerShotKind::WideFocusSub:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 192, 16, 16);
      break;
    case PlayerShotKind::HomingFocusMain:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 208, 16, 16);
      break;
    case PlayerShotKind::HomingFocusSub:
      src = Rect::FromLtwh((384 + ((display_angle + 8) & 0xf0)), 224, 16, 16);
      break;
    case PlayerShotKind::Count:
      continue;
    }

    GraphicsSurfaceBlit({x, y}, SurfaceId::System, src);
  }

  loadout_->DrawContinuousAttack(*this, focused_);
}
