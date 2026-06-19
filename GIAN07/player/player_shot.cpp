///
/// PlayerShot - Player shot pool management: fire dispatch, bullet
/// movement, collision, and drawing.
///
/// Attack form logic (FireMain / FireSub / FireBomb) lives in the
/// weapon/ strategy classes.  This file owns the shared shot pool,
/// damage table, and per-frame dispatch.
///

#include "player_shot.h"

#include "game/cast.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "gian.h"
#include "platform/graphics_backend.h"
#include "player.h"
#include "weapon/weapon_form.h"
#include <utility>

// --- Damage table indexed by bullet ID (t->c) ---
constexpr uint8_t TogeDamage[0x0c] = {
    // MainWeapon		// SubWeapon
    TDM_WIDE_MAIN,
    TDM_WIDE_SUB, // TYPE_A(WIDE)
    TDM_HOMING_MAIN,
    TDM_HOMING_SUB, // TYPE_B(HOMING)
    TDM_LASER_MAIN,
    TDM_LASER_SUB, // TYPE_C
    1,
    1, // For homing bomb
    TDM_WIDE_FOCUS_MAIN,
    TDM_WIDE_FOCUS_SUB, // TYPE_A focus (WIDE)
    TDM_HOMING_FOCUS_MAIN,
    TDM_HOMING_FOCUS_SUB // TYPE_B focus (HOMING)
};

// --- Fire dispatch ---

void Player::SetMaidShot() {
  // Start fire cooldown when the fire key is pressed.
  if (((Key_Data & KEY_TAMA) != 0) && toge_time == 0 &&
      muteki < MAID_MOVE_DISABLE_TIME) {
    toge_time = MAID_TAMA_START;
  }

  // Activate bomb if conditions are met.
  if (((Key_Data & KEY_BOMB) != 0) && (bomb_time == 0) &&
      (muteki == 0 || deathbomb_time != 0) &&
      (bomb != 0U) && (!Scroller.scene.MsgFlag)) {
    bomb_time = BaseForm_()->BombDuration();
    muteki = BOMBMUTEKI_VAL;
    bomb--;
    bomb_used++;
    if (deathbomb_time != 0) {
      deathbomb_count++;
      deathbomb_time = 0;
    }
    Ranking.Add(-25); // Difficulty down
  }

  // Bomb update (always uses the base form, not focus).
  if (bomb_time != 0U) {
    bomb_time--;
    BaseForm_()->FireBomb();
  }

  // Main / sub shot dispatch via the active (possibly focus) form.
  if (toge_time != 0U) {
    const uint8_t tier = (exp + 1) >> 5;
    if (IsMainShotFrame_(toge_time)) {
      ActiveForm_()->FireMain(tier);
    }
    if (IsSubShotFrame_(toge_time)) {
      ActiveForm_()->FireSub(tier);
    }
    toge_time--;
  }

  // Per-frame form tick (laser forms manage lay_time / lay_grp).
  BaseForm_()->OnFireTick();
}

// --- Bullet movement & hit check ---

void Player::MoveMaidShot() {
  int i = 0;

  for (i = 0; std::cmp_less(i, maid_tama_now); i++) {
    auto *t = &maid_tama[maid_tama_ind[i]];
    if (t->c == TID_HOMING_BOMB_B) {
      Enemies.DamageAt(t->x, t->y, TogeDamage[t->c]);
      t->count++;
      if (t->count >= 19) {
        t->flag = TF_DELETE;
      }
      continue;
    }
    if (t->effect == TE_NONE) {
      BulletManager::MoveByType(t);
      Bullets.MoveByOption(t);
      t->count++;
      if (((t->flag & TF_CLIP) == 0) && ((t->x) < GX_MIN || (t->x) > GX_MAX ||
                                          (t->y) < GY_MIN || (t->y) > GY_MAX)) {
        t->flag = TF_DELETE;
      }

      if (Enemies.DamageAt(t->x, t->y, TogeDamage[t->c])) {
        if (t->c == TID_HOMING_BOMB_A) {
          TamaSTDForm(TID_HOMING_BOMB_B);
          Bullets.command.type = T_SBHBOMB;
          TamaSetXY(t->x, t->y);
          TamaSetDeg(-64, 16);
          TamaSetSpd(10, 0);
          TamaSetNum(1, 0);
          SpawnShot_();
        }
        t->flag = TF_DELETE;
        Effects.SpawnFragment(t->x, t->y, FRG_HIT);
      }
    } else {
      {
        BulletManager::MoveByEffect(t);
      }
    }
  }
  Indsort(maid_tama_ind, maid_tama_now, maid_tama,
          [](const Bullet &t) { return (t.flag & TF_DELETE); });

  // Laser collision check
  if (weapon == 2 && (lay_grp != 0U)) {
    // Focus (low-speed) form: narrow the two beams and slightly raise damage.
    const bool focus = ((Key_Data & KEY_SHIFT) != 0);
    const int loff = focus ? (SBOPT_DX / 2) : SBOPT_DX;
    const int ldmg =
        focus ? ((lay_grp / 3) + 2) : ((lay_grp / 3) + 1);
    Enemies.DamageAt2(opx + (loff << 6), opy, ldmg);
    Enemies.DamageAt2(opx - (loff << 6), opy, ldmg);
  }
}

// --- Bullet drawing ---

void Player::DrawMaidShot() {
  int i = 0;
  int x = 0;
  int y = 0;
  PIXEL_LTRB src;
  PIXEL_LTRB ltemp;
  static PIXEL_LTRB HomingBomb[5] = {{520, 104, 520 + 8, 104 + 8},
                                     {528, 104, 528 + 16, 104 + 16},
                                     {544, 104, 544 + 24, 104 + 24},
                                     {568, 104, 568 + 32, 104 + 32},
                                     {600, 104, 600 + 40, 104 + 40}};

  for (i = 0; std::cmp_less(i, maid_tama_now); i++) {
    auto *t = &maid_tama[maid_tama_ind[i]];

    x = (t->x >> 6) - 8;
    y = (t->y >> 6) - 8;

    switch (t->c) {
    case TID_WIDE_MAIN:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 176, 16, 16};
      break;
    case TID_WIDE_SUB:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 192, 16, 16};
      break;
    case TID_HOMING_MAIN:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 208, 16, 16};
      break;
    case TID_HOMING_SUB:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 224, 16, 16};
      break;
    case TID_HOMING_BOMB_A:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 288, 16, 16};
      break;
    case TID_LASER_SUB:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 256, 16, 16};
      break;

    case TID_HOMING_BOMB_B:
      src = HomingBomb[(t->count / 4) % 5];
      break;

    // Focus (low-speed) form shots reuse the base-form sprite rows.
    case TID_WIDE_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 176, 16, 16};
      break;
    case TID_WIDE_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 192, 16, 16};
      break;
    case TID_HOMING_FOCUS_MAIN:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 208, 16, 16};
      break;
    case TID_HOMING_FOCUS_SUB:
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 224, 16, 16};
      break;
    }

    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  // Laser drawing
  if (weapon == 2 && (lay_grp != 0U)) {
    // Focus (low-speed) form: pull the two beams closer together.
    const int loff =
        ((Key_Data & KEY_SHIFT) != 0) ? (SBOPT_DX / 2) : SBOPT_DX;
    ltemp = PIXEL_LTWH{(384 + ((lay_grp - 1) << 4)), 240, 8, 16};

    x = (opx >> 6) + 4 - 8 + loff;
    y = (opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (opx >> 6) + 4 - 8 - loff;
    y = (opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    ltemp = PIXEL_LTWH{(384 + 8 + ((lay_grp - 1) << 4)), 240, 8, 16};
    for (i = (opy >> 6) - 36; i > -16; i -= 16) {
      x = (opx >> 6) + 4 - 8 + loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (i = (opy >> 6) - 36; i > -16; i -= 16) {
      x = (opx >> 6) + 4 - 8 - loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

// --- Shot pool initialization ---

void Player::SetMaidShotIndices() {
  for (int i = 0; i < MAIDTAMA_MAX; i++) {
    maid_tama_ind[i] = i;
  }
  maid_tama_now = 0;
}

// --- Laser fire trigger ---

void Player::SetMLaser(uint16_t time) {
  if ((Players.bomb_time != 0U) ||
      Players.muteki > MAID_MOVE_DISABLE_TIME) {
    Players.lay_time = 0;
    Players.lay_grp = 0;
    return;
  }

  if (Players.lay_time == 0) {
    Players.lay_time = time;
    Snd_SEPlay(SOUND_ID_SBLASER, Players.x);
  }
}
