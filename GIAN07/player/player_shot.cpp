///
/// PlayerShot - thin delegation layer.
///
/// The maid shot pool, fire dispatch, bullet movement, collision and
/// drawing now live in `bullets::BulletSubsystem`.  Player methods here
/// simply forward to `gWorld().projectiles.Bullets()` and to the active
/// `WeaponForm` strategy objects.
///

#include <utility>

#include "player_shot.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "core/world.h"
#include "player.h"
#include "sys/input.h"
#include "weapon/weapon_form.h"

// --- Fire dispatch ---

void Player::SetMaidShot() {
  // Start fire cooldown when the fire key is pressed.
  if (((Key_Data & KEY_TAMA) != 0) && toge_time_ == 0 &&
      muteki_ < MAID_MOVE_DISABLE_TIME) {
    toge_time_ = MAID_TAMA_START;
  }

  // Activate bomb_ if conditions are met.
  if (((Key_Data & KEY_BOMB) != 0) && (bomb_time_ == 0) &&
      (muteki_ == 0 || deathbomb_time_ != 0) && (bomb_ != 0U) &&
      (!Scroller.scene.MsgFlag)) {
    bomb_time_ = BaseForm_()->BombDuration();
    muteki_ = BOMBMUTEKI_VAL;
    bomb_--;
    bomb_used_++;
    if (deathbomb_time_ != 0) {
      deathbomb_count_++;
      deathbomb_time_ = 0;
    }
    Ranking.Add(-BOMB_RANK_DECR); // Difficulty down
  }

  // Bomb update (always uses the base form, not focus).
  if (bomb_time_ != 0U) {
    bomb_time_--;
    BaseForm_()->FireBomb();
  }

  // Main / sub shot dispatch via the active (possibly focus) form.
  if (toge_time_ != 0U) {
    const uint8_t tier = (exp_ + 1) >> 5;
    if (IsMainShotFrame_(toge_time_)) {
      ActiveForm_()->FireMain(tier);
    }
    if (IsSubShotFrame_(toge_time_)) {
      ActiveForm_()->FireSub(tier);
    }
    toge_time_--;
  }

  // Per-frame form tick (laser forms manage lay_time_ / lay_grp_).
  BaseForm_()->OnFireTick();
}

// --- Maid shot pool: forward to ProjectileSystem ---

void Player::MoveMaidShot() {
  gWorld().projectiles.Bullets().MovePlayer();
  // Weapon-specific continuous-beam collision (laser).
  ActiveForm_()->OnCollisionTick();
}

void Player::DrawMaidShot() {
  gWorld().projectiles.Bullets().DrawPlayer();

  // Continuous laser beam drawing — owned by the player rather than the
  // shared shot pool, as it draws an infinite vertical beam directly from
  // the option positions and depends on player input/state (weapon_, shift).
  if (weapon_ == 2 && (lay_grp_ != 0U)) {
    PIXEL_LTRB ltemp;
    int x = 0, y = 0;

    // Focus (low-speed) form: pull the two beams closer together.
    const int loff = ((Key_Data & KEY_SHIFT) != 0) ? (SBOPT_DX / 2) : SBOPT_DX;

    // Beam heads near the options.
    ltemp = PIXEL_LTWH{(384 + ((lay_grp_ - 1) << 4)), 240, 8, 16};
    x = (opx_ >> 6) + 4 - 8 + loff;
    y = (opy_ >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (opx_ >> 6) + 4 - 8 - loff;
    y = (opy_ >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    // Beam tails continuously blitted above the heads.
    ltemp = PIXEL_LTWH{(384 + 8 + ((lay_grp_ - 1) << 4)), 240, 8, 16};
    for (int i = (opy_ >> 6) - 36; i > -16; i -= 16) {
      x = (opx_ >> 6) + 4 - 8 + loff;
      GrpSurface_Blit({x, i}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (int i = (opy_ >> 6) - 36; i > -16; i -= 16) {
      x = (opx_ >> 6) + 4 - 8 - loff;
      GrpSurface_Blit({x, i}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

void Player::SetMaidShotIndices() {
  gWorld().projectiles.Bullets().ResetPlayerIndices();
}

// --- Laser fire trigger ---

void Player::SetMLaser(uint16_t time) {
  if ((Players.bomb_time_ != 0U) || Players.muteki_ > MAID_MOVE_DISABLE_TIME) {
    Players.lay_time_ = 0;
    Players.lay_grp_ = 0;
    return;
  }

  if (Players.lay_time_ == 0) {
    Players.lay_time_ = time;
  }
}

// Re-direct to the shared ProjectileSystem so WeaponForm subclasses
// can call FireMain()/FireSub()/FireBomb() without touching globals.
bullets::BulletSubsystem &Player::Bullets() {
  return gWorld().projectiles.Bullets();
}

uint16_t Player::ShotCount() const {
  return gWorld().projectiles.Bullets().PlayerNow();
}