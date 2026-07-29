///
/// Player projectiles - movement, collision, and drawing.
///
/// Loadout-specific firing lives in player/loadout. This file owns the shared
/// projectile pool and per-frame dispatch.
///

#include <array>
#include <utility>

#include "loadout/player_loadout.h"
#include "player.h"
#include "player_attack.h"
#include "player_shot.h"

#include "audio/snd.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/graphics_backend.h"
#include "stage/stage_session.h"
#include "sys/input.h"
#include "util/cast.h"
#include "util/ut_math.h"

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

constexpr uint8_t ShotDamage(PlayerShotKind kind) {
  return kShotDamage[std::to_underlying(kind)];
}
} // namespace

// --- Fire dispatch ---

void Player::UpdateWeapons(EnemyManager &enemies, INPUT_BITS input) {
  if (((input & KEY_TAMA) != 0) && toge_time_ == 0 && !IsMovementDisabled()) {
    toge_time_ = kShotCycleStart;
  }

  if ((input & KEY_BOMB) != 0) {
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
    const uint8_t tier = (exp_ + 1) >> 5;
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
  short deg_t = 0;

  switch (motion_) {
  case PlayerShotMotion::Straight:
    x_ += velocity_x_;
    y_ += velocity_y_;
    return false;

  case PlayerShotMotion::Homing: {
    const bool spawn_smoke = (age_ & 1) != 0;
    const int previous_x = x_;
    const int previous_y = y_;
    x_ += velocity_x_;
    y_ += velocity_y_;
    if (age_ < 70 && target.active) {
      deg_t = atan8(target.x - previous_x, target.y - previous_y) - direction_;
    } else if (age_ < 70) {
      deg_t = atan8(0, -20_px - previous_y) - direction_;
    } else {
      deg_t = 0;
    }
    if (deg_t < -128) {
      deg_t += 256;
    }
    if (deg_t > 128) {
      deg_t -= 256;
    }
    if (deg_t == 0) {
      if (turn_rate_ != 0) {
        turn_rate_--;
      }
      speed_ += acceleration_;
    } else {
      if (turn_rate_ < INT8_MAX) {
        turn_rate_++;
      }
      speed_ -= acceleration_;
    }
    direction_ += (deg_t * Cast::sign<uint8_t>(turn_rate_) / 255);
    velocity_x_ = cosl(direction_, speed_);
    velocity_y_ = sinl(direction_, speed_);
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
      enemies.ApplyPlayerAttack(PlayerAttack::Point(
          WORLD_POINT::FromWorld(t.x_, t.y_), ShotDamage(t.kind_)));
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

    if (enemies.ApplyPlayerAttack(PlayerAttack::Point(
            WORLD_POINT::FromWorld(t.x_, t.y_), ShotDamage(t.kind_)))) {
      if (t.kind_ == PlayerShotKind::HomingBomb) {
        PlayerShotSpawnInfo si{
            .x = t.x_,
            .y = t.y_,
            .direction = 192,
            .direction_step = 16,
            .count = 1,
            .speed = 2.5_px,
            .acceleration = 0,
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
  PIXEL_LTRB src;
  static constexpr PIXEL_LTRB HomingBomb[5] = {{520, 104, 520 + 8, 104 + 8},
                                               {528, 104, 528 + 16, 104 + 16},
                                               {544, 104, 544 + 24, 104 + 24},
                                               {568, 104, 568 + 32, 104 + 32},
                                               {600, 104, 600 + 40, 104 + 40}};

  for (const auto &t : maid_tama_) {

    x = (t.x_ >> 6) - 8;
    y = (t.y_ >> 6) - 8;

    switch (t.kind_) {
    case PlayerShotKind::WideMain:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 176, 16, 16};
      break;
    case PlayerShotKind::WideSub:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 192, 16, 16};
      break;
    case PlayerShotKind::HomingMain:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 208, 16, 16};
      break;
    case PlayerShotKind::HomingSub:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 224, 16, 16};
      break;
    case PlayerShotKind::HomingBomb:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 288, 16, 16};
      break;
    case PlayerShotKind::LaserSub:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 256, 16, 16};
      break;
    case PlayerShotKind::HomingBombBlast:
      src = HomingBomb[(static_cast<int>(t.age_) / 4) % 5];
      break;
    case PlayerShotKind::WideFocusMain:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 176, 16, 16};
      break;
    case PlayerShotKind::WideFocusSub:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 192, 16, 16};
      break;
    case PlayerShotKind::HomingFocusMain:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 208, 16, 16};
      break;
    case PlayerShotKind::HomingFocusSub:
      src = PIXEL_LTWH{(384 + ((t.direction_ + 8) & 0xf0)), 224, 16, 16};
      break;
    case PlayerShotKind::Count:
      continue;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  loadout_->DrawContinuousAttack(*this, focused_);
}
