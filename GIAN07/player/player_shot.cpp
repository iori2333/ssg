///
/// PlayerShot — Player shot pool management: fire dispatch, bullet
/// movement, collision, and drawing.
///
/// Attack form logic (FireMain / FireSub / FireBomb) lives in the
/// weapon_/ strategy classes.  This file owns the shared shot pool,
/// damage table, and per-frame dispatch.
///

#include "player_shot.h"

#include "audio/snd.h"
#include "core/gian.h"
#include "effect/effect_manager.h"
#include "effect/fragment.h"
#include "enemy/enemy_manager.h"
#include "core/game_manager.h"
#include "gfx/graphics_backend.h"
#include "player.h"
#include "stage/scroll_manager.h"
#include "sys/input.h"
#include "util/cast.h"
#include "util/ut_math.h"
#include "weapon/weapon_form.h"

// --- Damage table indexed by bullet ID (t->c) ---
constexpr uint8_t TogeDamage[0x0c] = {
    TDM_WIDE_MAIN,        TDM_WIDE_SUB,   TDM_HOMING_MAIN,
    TDM_HOMING_SUB,       TDM_LASER_MAIN, TDM_LASER_SUB,
    1,                    1,
    TDM_WIDE_FOCUS_MAIN,  TDM_WIDE_FOCUS_SUB,
    TDM_HOMING_FOCUS_MAIN, TDM_HOMING_FOCUS_SUB,
};

// --- Fire dispatch ---

void Player::SetMaidShot() {
  if (((Key_Data & KEY_TAMA) != 0) && toge_time_ == 0 &&
      muteki_ < MAID_MOVE_DISABLE_TIME) {
    toge_time_ = MAID_TAMA_START;
  }

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
    game_->AddRank(-25);
  }

  if (bomb_time_ != 0U) {
    bomb_time_--;
    BaseForm_()->FireBomb();
  }

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

  BaseForm_()->OnFireTick();
}

// --- Movement helpers ---

void PlayerShot::MoveByType() {
  short deg_t = 0;

  switch (type_) {
  case 0: // T_NORM
    tx_ += vx_;
    ty_ += vy_;
    return;

  case 9: { // T_SBHOMING
    if ((count_ & 1) != 0) {
      Effects.SpawnFragment(x_, y_, FRG_SMOKE);
    }
    tx_ += vx_;
    ty_ += vy_;
    if (count_ < 70 && Enemies.homing_flag != HOMING_DUMMY) {
      deg_t = atan8(Enemies.homing_x - x_, Enemies.homing_y - y_) - d_;
    } else if (count_ < 70) {
      deg_t = atan8(0, (-20 * 64) - y_) - d_;
    } else {
      flag_ = 0;
      deg_t = 0;
    }
    if (deg_t < -128) {
      deg_t += 256;
    }
    if (deg_t > 128) {
      deg_t -= 256;
    }
    if (deg_t == 0) {
      if (vd_ != 0) {
        vd_--;
      }
      v_ += a_;
    } else {
      if (vd_ < INT8_MAX) {
        vd_++;
      }
      v_ -= a_;
    }
    d_ += (deg_t * Cast::sign<uint8_t>(vd_) / 255);
    vx_ = cosl(d_, v_);
    vy_ = sinl(d_, v_);
    return;
  }
  case 10: // T_SBHBOMB
    if (count_ >= 49) {
      flag_ |= PlayerFlag::DEL;
    }
    return;
  }
}

void PlayerShot::MoveByEffect() {
  switch (effect_ & 0xf0) {
  case 0x50: // TE_CIRCLE1
    x_ = (tx_ += (vx_ >> 1));
    y_ = (ty_ += (vy_ >> 1));
    if (count_ >= 19) {
      effect_ = 0;
    }
    return;
  case 0xf0: // TE_DELETE
    x_ += (vx_ >> 1);
    y_ += (vy_ >> 1);
    if (count_ >= 47) {
      flag_ |= PlayerFlag::DEL;
    }
    return;
  }
}

// --- Bullet movement & hit check ---

void Player::MoveMaidShot() {
  for (auto &t : maid_tama_) {
    if (t.c_ == TID_HOMING_BOMB_B) {
      Enemies.DamageAt(t.x_, t.y_, TogeDamage[t.c_]);
      t.count_++;
      if (t.count_ >= 19) {
        t.flag_ |= PlayerFlag::DEL;
      }
      continue;
    }
    if (t.effect_ == 0) {
      t.MoveByType();
      t.x_ = t.tx_;
      t.y_ = t.ty_;
      t.count_++;
      if (((t.flag_ & PlayerFlag::CLIP) == 0) && ((t.x_) < GX_MIN || (t.x_) > GX_MAX ||
                                        (t.y_) < GY_MIN || (t.y_) > GY_MAX)) {
        t.flag_ |= PlayerFlag::DEL;
      }

      if (Enemies.DamageAt(t.x_, t.y_, TogeDamage[t.c_])) {
        if (t.c_ == TID_HOMING_BOMB_A) {
          PlayerShotSpawnInfo si{.x = t.x_,
                                 .y = t.y_,
                                 .d = 192,
                                 .dw = 16,
                                 .n = 1,
                                 .v = SPEEDM(10),
                                 .a = 0,
                                 .c = TID_HOMING_BOMB_B,
                                 .type = 10};
          SpawnShot(si);
        }
        t.flag_ |= PlayerFlag::DEL;
        Effects.SpawnFragment(t.x_, t.y_, FRG_HIT);
      }
    } else {
      t.MoveByEffect();
    }
  }
  maid_tama_.Compact(
      [](const PlayerShot &t) { return static_cast<bool>(t.flag_ & PlayerFlag::DEL); });

  ActiveForm_()->OnCollisionTick();
}

// --- Bullet drawing ---

void Player::DrawMaidShot() {
  int x = 0;
  int y = 0;
  PIXEL_LTRB src;
  PIXEL_LTRB ltemp;
  static PIXEL_LTRB HomingBomb[5] = {{520, 104, 520 + 8, 104 + 8},
                                      {528, 104, 528 + 16, 104 + 16},
                                      {544, 104, 544 + 24, 104 + 24},
                                      {568, 104, 568 + 32, 104 + 32},
                                      {600, 104, 600 + 40, 104 + 40}};

  for (auto &t : maid_tama_) {

    x = (t.x_ >> 6) - 8;
    y = (t.y_ >> 6) - 8;

    switch (t.c_) {
    case TID_WIDE_MAIN:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 176, 16, 16};
      break;
    case TID_WIDE_SUB:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 192, 16, 16};
      break;
    case TID_HOMING_MAIN:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 208, 16, 16};
      break;
    case TID_HOMING_SUB:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 224, 16, 16};
      break;
    case TID_HOMING_BOMB_A:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 288, 16, 16};
      break;
    case TID_LASER_SUB:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 256, 16, 16};
      break;
    case TID_HOMING_BOMB_B:
      src = HomingBomb[(static_cast<int>(t.count_) / 4) % 5];
      break;
    case TID_WIDE_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 176, 16, 16};
      break;
    case TID_WIDE_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 192, 16, 16};
      break;
    case TID_HOMING_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 208, 16, 16};
      break;
    case TID_HOMING_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((t.d_ + 8) & 0xf0)), 224, 16, 16};
      break;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  if (weapon_ == 2 && (lay_grp_ != 0U)) {
    const int loff = ((Key_Data & KEY_SHIFT) != 0) ? (SBOPT_DX / 2) : SBOPT_DX;
    ltemp = PIXEL_LTWH{(384 + ((lay_grp_ - 1) << 4)), 240, 8, 16};

    x = (opx_ >> 6) + 4 - 8 + loff;
    y = (opy_ >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (opx_ >> 6) + 4 - 8 - loff;
    y = (opy_ >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    ltemp = PIXEL_LTWH{(384 + 8 + ((lay_grp_ - 1) << 4)), 240, 8, 16};
    for (int i = (opy_ >> 6) - 36; i > -16; i -= 16) {
      x = (opx_ >> 6) + 4 - 8 + loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (int i = (opy_ >> 6) - 36; i > -16; i -= 16) {
      x = (opx_ >> 6) + 4 - 8 - loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

// --- Shot pool initialization ---

void Player::SetMaidShotIndices() { maid_tama_.Init(); }

// --- Laser fire trigger ---

void Player::SetMLaser(uint16_t time) {
  if ((bomb_time_ != 0U) || muteki_ > MAID_MOVE_DISABLE_TIME) {
    lay_time_ = 0;
    lay_grp_ = 0;
    return;
  }

  if (lay_time_ == 0) {
    lay_time_ = time;
  }
}
