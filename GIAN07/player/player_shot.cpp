///
/// PlayerShot - Maid shot processing
///

#include "player_shot.h"

#include "game/cast.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "gian.h"
#include "platform/graphics_backend.h"
#include "player_manager.h"
#include <utility>

// [Secret functions]
static void MTamaSet();

static void SetT_A0(); // Maid shot TYPE-A
static void SetT_A1();
static void SetT_A2();
static void SetT_A3();
static void SetT_A4();
static void SetT_A5();
static void SetT_A6();
static void SetT_A7();
static void SetT_A8();

static void SetT_B0(); // Maid shot TYPE-B
static void SetT_B1();
static void SetT_B2();
static void SetT_B3();
static void SetT_B4();
static void SetT_B5();
static void SetT_B6();
static void SetT_B7();
static void SetT_B8();

static void SetT_C0(); // Maid shot TYPE-C
static void SetT_C1();
static void SetT_C2();
static void SetT_C3();
static void SetT_C4();
static void SetT_C5();
static void SetT_C6();
static void SetT_C7();
static void SetT_C8();

static void SetT_D0(); // Maid shot TYPE-D
static void SetT_D1();
static void SetT_D2();
static void SetT_D3();
static void SetT_D4();
static void SetT_D5();
static void SetT_D6();
static void SetT_D7();
static void SetT_D8();

static void SetT_FA0(); // Maid shot TYPE-A (focus / low-speed form)
static void SetT_FA1();
static void SetT_FA2();
static void SetT_FA3();
static void SetT_FA4();
static void SetT_FA5();
static void SetT_FA6();
static void SetT_FA7();
static void SetT_FA8();

static void SetT_FB0(); // Maid shot TYPE-B (focus / low-speed form)
static void SetT_FB1();
static void SetT_FB2();
static void SetT_FB3();
static void SetT_FB4();
static void SetT_FB5();
static void SetT_FB6();
static void SetT_FB7();
static void SetT_FB8();

static void SetT_FC0(); // Maid shot TYPE-C (focus / low-speed form)
static void SetT_FC1();
static void SetT_FC2();
static void SetT_FC3();
static void SetT_FC4();
static void SetT_FC5();
static void SetT_FC6();
static void SetT_FC7();
static void SetT_FC8();

static void SetWideBomb();
static void SetHomingBomb();
static void SetLaserBomb();
static void SetCactusBomb();

// maid_tama[], maid_tama_ind[], maid_tama_now moved to PlayerManager in player_manager.cpp

constexpr uint8_t TogeDamage[0x0c] = {
    // Indexed by bullet ID (TID_*).
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

static void (*MaidTamaFunc[4][9])(void) = {
    {SetT_A0, SetT_A1, SetT_A2, SetT_A3, SetT_A4, SetT_A5, SetT_A6, SetT_A7,
     SetT_A8},
    {SetT_B0, SetT_B1, SetT_B2, SetT_B3, SetT_B4, SetT_B5, SetT_B6, SetT_B7,
     SetT_B8},
    {SetT_C0, SetT_C1, SetT_C2, SetT_C3, SetT_C4, SetT_C5, SetT_C6, SetT_C7,
     SetT_C8},
    {SetT_D0, SetT_D1, SetT_D2, SetT_D3, SetT_D4, SetT_D5, SetT_D6, SetT_D7,
     SetT_D8}};

// Dispatch table for the focus (low-speed) attack form. TYPE-D has no focus
// form and falls back to the empty TYPE-D slots.
static void (*MaidFocusTamaFunc[4][9])(void) = {
    {SetT_FA0, SetT_FA1, SetT_FA2, SetT_FA3, SetT_FA4, SetT_FA5, SetT_FA6,
     SetT_FA7, SetT_FA8},
    {SetT_FB0, SetT_FB1, SetT_FB2, SetT_FB3, SetT_FB4, SetT_FB5, SetT_FB6,
     SetT_FB7, SetT_FB8},
    {SetT_FC0, SetT_FC1, SetT_FC2, SetT_FC3, SetT_FC4, SetT_FC5, SetT_FC6,
     SetT_FC7, SetT_FC8},
    {SetT_D0, SetT_D1, SetT_D2, SetT_D3, SetT_D4, SetT_D5, SetT_D6, SetT_D7,
     SetT_D8}};

static void (*MaidBombFunc[4])(void) = {SetWideBomb, SetHomingBomb,
                                        SetLaserBomb, SetCactusBomb};

static constexpr auto WIDE_BOMB_TIME = (60 * 4);
static constexpr auto HOMING_BOMB_TIME = (60 * 3);
static constexpr auto LASER_BOMB_TIME = (60 * 2);
static constexpr auto CACTUS_BOMB_TIME = 0;

static constexpr uint8_t MaidBombTime[4] = {WIDE_BOMB_TIME, HOMING_BOMB_TIME,
                                            LASER_BOMB_TIME, CACTUS_BOMB_TIME};

static constexpr auto MAID_TAMA_START = 18; // 12
static constexpr auto MAID_MAIN_SHOT = 6;   // 4
static constexpr auto MAID_SUB_SHOT = 9;    // 6

// Fire shot!
void PlayerManager::SetMaidShot() {
  // This function checks the previous fire state (Viv_St).
  // If fire is possible, it fires; otherwise, it simply returns.
  // Uses functions compatible with TAMA.cpp for setting bullets.

  if (((Key_Data & KEY_TAMA) != 0) && viv.toge_time == 0 &&
      viv.muteki < MAID_MOVE_DISABLE_TIME) {
    viv.toge_time = MAID_TAMA_START;
  }

  // Activate bomb if conditions are met
  if (((Key_Data & KEY_BOMB) != 0) && (viv.bomb_time == 0) &&
      (viv.muteki == 0 || viv.deathbomb_time != 0) && // No bomb during invincibility (except deathbomb window)
      (viv.bomb != 0U) && (!Scroller.scene.MsgFlag)) {
    // if(viv.weapon == 0) EnterBombPalette();

    viv.bomb_time = MaidBombTime[viv.weapon & 3]; // Change per equipment
    viv.muteki = BOMBMUTEKI_VAL;
    viv.bomb--;
    viv.bomb_used++;
    if (viv.deathbomb_time != 0) {
      viv.deathbomb_count++;
      viv.deathbomb_time = 0;
    }
    Ranking.Add(-25); // Difficulty down
  }

  if (viv.bomb_time != 0U) {
    viv.bomb_time--;
    MaidBombFunc[viv.weapon]();

    // if(viv.bomb_time == 0 && viv.weapon == 0)
    //	LeaveBombPalette();
  }

  if (viv.toge_time != 0U) {
    // Focus (low-speed) form: holding KEY_SHIFT swaps the main shot to a
    // per-weapon focus pattern. The selected weapon (and bombs) are unchanged.
    if ((Key_Data & KEY_SHIFT) != 0) {
      MaidFocusTamaFunc[viv.weapon & 3][(viv.exp + 1) >> 5]();
    } else {
      MaidTamaFunc[viv.weapon & 3][(viv.exp + 1) >> 5]();
    }
    viv.toge_time--;
  }

  // When laser is equipped
  if (viv.weapon == 2 && (viv.lay_time != 0U)) {
    viv.lay_time--;
    if (viv.lay_time < 64) {
      viv.lay_grp = 0;
    } else if (viv.lay_time < 64 + 50) {
      viv.lay_grp = 1;
    } else if (viv.lay_time < 64 + 100) {
      viv.lay_grp = 2;
    } else if (viv.lay_time < 64 + 150) {
      viv.lay_grp = 3;
    } else {
      viv.lay_grp = 4;
    }
    // viv.lay_grp = (viv.lay_time+63)>>6;
  }
}

// Bullet movement & hit check
void PlayerManager::MoveMaidShot() {
  // Uses enemy bullet movement from TAMA.cpp.
  // Collision detection is against enemies!
  // Collision is checked by passing coordinates to functions in ENEMY.cpp.

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
          MTamaSet();
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
  if (viv.weapon == 2 && (viv.lay_grp != 0U)) {
    // x = (viv.opx>>6)+4 -8 + SBOPT_DX;
    // y = (viv.opy>>6)-20;
    // Focus (low-speed) form: narrow the two beams and slightly raise damage.
    const bool focus = ((Key_Data & KEY_SHIFT) != 0);
    const int loff = focus ? (SBOPT_DX / 2) : SBOPT_DX;
    const int ldmg =
        focus ? ((viv.lay_grp / 3) + 2) : ((viv.lay_grp / 3) + 1);
    Enemies.DamageAt2(viv.opx + (loff << 6), viv.opy, ldmg);
    Enemies.DamageAt2(viv.opx - (loff << 6), viv.opy, ldmg);
  }
}

// Bullet drawing
void PlayerManager::DrawMaidShot() {
  // Cannot use TAMA.cpp functions here, so implement custom drawing.

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

    x = (t->x >> 6) - 8; // -8 is for coordinate correction
    y = (t->y >> 6) - 8; // Same as above

    // Set drawing rectangle based on bullet type
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

    // Full clipping with bounds check
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  // Laser drawing
  if (viv.weapon == 2 && (viv.lay_grp != 0U)) {
    // Focus (low-speed) form: pull the two beams closer together.
    const int loff =
        ((Key_Data & KEY_SHIFT) != 0) ? (SBOPT_DX / 2) : SBOPT_DX;
    ltemp = PIXEL_LTWH{(384 + ((viv.lay_grp - 1) << 4)), 240, 8, 16};

    x = (viv.opx >> 6) + 4 - 8 + loff;
    y = (viv.opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (viv.opx >> 6) + 4 - 8 - loff;
    y = (viv.opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    ltemp = PIXEL_LTWH{(384 + 8 + ((viv.lay_grp - 1) << 4)), 240, 8, 16};
    for (i = (viv.opy >> 6) - 36; i > -16; i -= 16) {
      x = (viv.opx >> 6) + 4 - 8 + loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (i = (viv.opy >> 6) - 36; i > -16; i -= 16) {
      x = (viv.opx >> 6) + 4 - 8 - loff;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

// Bullet hash table initialization
void PlayerManager::SetMaidShotIndices() {
  int i = 0;

  // Initializing this array initializes all bullets
  for (i = 0; i < MAIDTAMA_MAX; i++) {
    maid_tama_ind[i] = i;
    // memset(Players.maid_tama+i,0,sizeof(TAMA_DATA));
  }

  // Don't forget to zero-initialize the current count
  maid_tama_now = 0;
}

static void MTamaSet() {
  for (decltype(Bullets.command.n) i = 0; i < Bullets.command.n; i++) {
    if (Players.maid_tama_now + 1 >= MAIDTAMA_MAX) {
      return; // Cannot set
    }

    auto *t =
        &Players.maid_tama[Players.maid_tama_ind
                               [Players.maid_tama_now++]]; // Set bullet pointer

    t->x = t->tx = Bullets.command.x; // Set X coordinate
    t->y = t->ty = Bullets.command.y; // Set Y coordinate

    t->v = t->v0 = Bullets.Speed(i); // Set initial speed
    t->a = Bullets.command.a;        // Note: size is char

    t->d = Bullets.Dir(i); // Bullet launch angle
    t->d16 = (t->d << 8);  // Used for angular velocity movement

    t->vx = cosl(t->d, t->v); // Set velocity X component
    t->vy = sinl(t->d, t->v); // Set velocity Y component

    t->vd = Bullets.command.vd;         // Angular velocity or homing rate
    t->c = Bullets.command.c;           // Bullet ID
    t->rep = Bullets.command.rep;       // Repeat count
    t->type = Bullets.command.type;     // Bullet type
    t->option = Bullets.command.option; // Bullet attributes (vibe, reflect, etc.)
    t->effect = 0;                      // Bullets.command.cmd & 0xf0;			//
                                        // Bullet effect
    t->count = 0;                       // Initialize counter
    t->flag = Bullets.Flag();           // Initialize flag
  }
}

inline bool IsMainShot(uint16_t t) {
  return (t == MAID_MAIN_SHOT || t == MAID_MAIN_SHOT * 2 ||
          t == MAID_MAIN_SHOT * 3);
}
inline bool IsSubShot(uint16_t t) {
  return (t == 0 || t == MAID_SUB_SHOT) && Players.viv.bomb_time == 0;
}

void PlayerManager::SetMLaser(uint16_t time) {
  if ((Players.viv.bomb_time != 0U) ||
      Players.viv.muteki > MAID_MOVE_DISABLE_TIME) {
    Players.viv.lay_time = 0;
    Players.viv.lay_grp = 0;
    return;
  }

  if (Players.viv.lay_time == 0) {
    Players.viv.lay_time = time;
    Snd_SEPlay(SOUND_ID_SBLASER, Players.viv.x);
  }
}

// Shot TYPE-A
static void SetT_A0() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Single center shot only
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_A1() {
  char dd = 0;

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 5, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }

  if (IsMainShot(Players.viv.toge_time)) {
    // Lightly spread main shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_A2() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2-shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));

    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetDeg(-64 + 5, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }
}

static void SetT_A3() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-way shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 4);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 5, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }
}

static void SetT_A4() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-way shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 4);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 8, 7); //(-64+5,7);
    TamaSetSpd(54, 0);
    TamaSetNum(2, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 8, 7); //(-64-5,7);
    MTamaSet();
  }
}

static void SetT_A5() { SetT_A4(); }

static void SetT_A6() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-way shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 3);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 10, 8); //-64+6,4);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 10, 8); //(-64-6,4);
    MTamaSet();
  }
}

static void SetT_A7() { SetT_A6(); }

static void SetT_A8() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-way shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 6));
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 3);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 12, 8); //(-64+7,4);
    TamaSetSpd(54, 0);
    TamaSetNum(4, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 12, 8); //(-64-7,4);
    MTamaSet();
  }
}

// Shot TYPE-B
static void SetT_B0() {
  char dd = 0;

  if (IsMainShot(Players.viv.toge_time)) {
    // Lightly spread main shot
    Players.viv.toge_ex += 32;
    dd = Cast::down<int8_t>(sinl(Players.viv.toge_ex, 4));
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64 + dd, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
  // Players.viv.toge_time = 3;
}

static void SetT_B1() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2-shot
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }
  // Players.viv.toge_time = 4;
  // if((++Players.viv.toge_ex)&7) return;

  if (IsSubShot(Players.viv.toge_time)) {
    // Homing bullet
    // Option shot (right)
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B2() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-way shot
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  // Players.viv.toge_time = 4;
  // if((++Players.viv.toge_ex)&7) return;

  if (IsSubShot(Players.viv.toge_time)) {
    // Homing bullet
    // Option shot (right)
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B3() { SetT_B2(); }

static void SetT_B4() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 5-way shot
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  // Players.viv.toge_time = 4;
  // if((++Players.viv.toge_ex)&7) return;

  if (IsSubShot(Players.viv.toge_time)) {
    // Homing bullet
    // Option shot (right)
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B5() { SetT_B4(); }

static void SetT_B6() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 5-way shot
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  // Players.viv.toge_time = 4;
  // if((++Players.viv.toge_ex)&3) return;

  if (IsSubShot(Players.viv.toge_time)) {
    // Homing bullet
    // Option shot (right)
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B7() { SetT_B6(); }

static void SetT_B8() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 5-way shot
    TamaSTDForm(TID_HOMING_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 7);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  // Players.viv.toge_time = 4;
  // if((++Players.viv.toge_ex)&3) return;

  if (IsSubShot(Players.viv.toge_time)) {
    // Homing bullet
    // Option shot (right)
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 22, 30);
    TamaSetNum(2, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(64 + 22, 30);
    MTamaSet();
  }
}

// Shot TYPE-C
static void SetT_C0() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Single center shot only
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_C1() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2-column shot
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 50);
}

static void SetT_C2() { SetT_C1(); }

static void SetT_C3() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-way shot
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 100);
}

static void SetT_C4() { SetT_C3(); }

static void SetT_C5() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-way shot (center in 2 columns)
    TamaSTDForm(TID_LASER_SUB);
    TamaSetSpd(54, 0);

    TamaSetDeg(-64 - 5, 10);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetNum(2, 0);
    MTamaSet();

    TamaSetDeg(-64 + 5, 10);
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 150);
}

static void SetT_C6() { SetT_C5(); }

static void SetT_C7() { SetT_C5(); }

static void SetT_C8() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 5-way shot
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 200);
}

// Shot TYPE-D
static void SetT_D0() {}

static void SetT_D1() {}

static void SetT_D2() {}

static void SetT_D3() {}

static void SetT_D4() {}

static void SetT_D5() {}

static void SetT_D6() {}

static void SetT_D7() {}

static void SetT_D8() {}

// Shot TYPE-A focus (WIDE -> narrow dense cone)
static void SetT_FA0() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Single center shot only
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_FA1() {
  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 2, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 2, 0);
    MTamaSet();
  }

  if (IsMainShot(Players.viv.toge_time)) {
    // Single straight shot
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_FA2() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2 parallel straight columns
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetDeg(-64 + 2, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 2, 0);
    MTamaSet();
  }
}

static void SetT_FA3() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3 parallel straight columns
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (12 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 2, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 2, 0);
    MTamaSet();
  }
}

static void SetT_FA4() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3 parallel straight columns
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (12 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) 2-way (narrow)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 2, 1);
    TamaSetSpd(54, 0);
    TamaSetNum(2, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 2, 1);
    MTamaSet();
  }
}

static void SetT_FA5() { SetT_FA4(); }

static void SetT_FA6() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4 parallel straight columns
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (18 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) 3-way (narrow)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 3, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 3, 2);
    MTamaSet();
  }
}

static void SetT_FA7() { SetT_FA6(); }

static void SetT_FA8() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4 parallel straight columns
    TamaSTDForm(TID_WIDE_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (18 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) 4-way (narrow)
    TamaSTDForm(TID_WIDE_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 + 4, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(4, 0);
    MTamaSet();

    // Option shot (left)
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64 - 4, 2);
    MTamaSet();
  }
}

// Shot TYPE-B focus (HOMING -> high-power straight-line columns, no tracking)
static void SetT_FB0() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Single center shot only
    TamaSTDForm(TID_HOMING_FOCUS_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_FB1() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2-column straight shot
    TamaSTDForm(TID_HOMING_FOCUS_MAIN);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) straight
    TamaSTDForm(TID_HOMING_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left) straight
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    MTamaSet();
  }
}

static void SetT_FB2() { SetT_FB1(); }

static void SetT_FB3() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-column straight shot
    TamaSTDForm(TID_HOMING_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (12 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) straight
    TamaSTDForm(TID_HOMING_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left) straight
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    MTamaSet();
  }
}

static void SetT_FB4() { SetT_FB3(); }

static void SetT_FB5() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-column straight shot
    TamaSTDForm(TID_HOMING_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (18 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) straight
    TamaSTDForm(TID_HOMING_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left) straight
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    MTamaSet();
  }
}

static void SetT_FB6() { SetT_FB5(); }

static void SetT_FB7() { SetT_FB5(); }

static void SetT_FB8() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-column straight shot
    TamaSTDForm(TID_HOMING_FOCUS_MAIN);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetXY(Players.viv.x - (18 * 64), Players.viv.y);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  if (IsSubShot(Players.viv.toge_time)) {
    // Option shot (right) straight
    TamaSTDForm(TID_HOMING_FOCUS_SUB);
    TamaSetXY(Players.viv.opx + (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // Option shot (left) straight
    TamaSetXY(Players.viv.opx - (SBOPT_DX * 64), Players.viv.opy);
    TamaSetDeg(-64, 0);
    MTamaSet();
  }
}

// Shot TYPE-C focus (LASER -> narrowed main shot + narrowed laser beams)
static void SetT_FC0() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Single center shot only
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_FC1() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 2-column shot
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 50);
}

static void SetT_FC2() { SetT_FC1(); }

static void SetT_FC3() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 3-way shot (narrow)
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 100);
}

static void SetT_FC4() { SetT_FC3(); }

static void SetT_FC5() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 4-way shot (center in 2 columns, narrow)
    TamaSTDForm(TID_LASER_SUB);
    TamaSetSpd(54, 0);

    TamaSetDeg(-64 - 2, 4);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetNum(2, 0);
    MTamaSet();

    TamaSetDeg(-64 + 2, 4);
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 150);
}

static void SetT_FC6() { SetT_FC5(); }

static void SetT_FC7() { SetT_FC5(); }

static void SetT_FC8() {
  if (IsMainShot(Players.viv.toge_time)) {
    // Center 5-way shot (narrow)
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 2);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  PlayerManager::SetMLaser(64 + 200);
}

static void SetWideBomb() {
  int dx = 0;
  int dy = 0;
  int l = 0;

  if (Players.viv.bomb_time > WIDE_BOMB_TIME - 30) {
    return;
  }

  const auto d = Cast::down<uint8_t>(Players.viv.bomb_time * 3U);
  l = (WIDE_BOMB_TIME - Players.viv.bomb_time) * 26; // 16-32
  dx = GX_MID + (64 * 70 / 2) + cosl(d, l << 1);
  dy = GY_MID - (64 * 90 / 2) + sinl(d << 1, l);

  Effects.SpawnFragment(dx, dy, FRG_STAR1);
  Effects.SpawnFragment(dx, dy, FRG_STAR1);
  Effects.SpawnFragment(dx, dy, FRG_STAR2);

  Enemies.DamageAll(1);
}

static void SetHomingBomb() {
  if (Players.viv.bomb_time % 30 == 1) {
    TamaSTDForm(TID_HOMING_BOMB_A);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetSpd(28, 4);
    TamaSetDeg(64, 16);
    TamaSetNum(8, 1);
    MTamaSet();

    // Defective, discontinued
    // ObjectLockOn(&HomingX, &HomingY, 32*64, 32*64);
  }
}

// This acts more like a HitCheck than a Set
static void SetLaserBomb() {
  int ox = 0;
  int oy = 0;
  int i = 0;

  const auto LaserDeg = GetLaserDeg();

  ox = Players.viv.opx + (SBOPT_DX * 64);
  oy = Players.viv.opy;
  for (i = -3; i <= 3; i++) {
    const auto d = GetRightLaserDeg(LaserDeg, i);
    Enemies.DamageAt3(ox, oy, d);
  }

  ox = Players.viv.opx - (SBOPT_DX * 64);
  oy = Players.viv.opy;
  for (i = -3; i <= 3; i++) {
    const auto d = GetLeftLaserDeg(LaserDeg, i);
    Enemies.DamageAt3(ox, oy, d);
  }
}

static void SetCactusBomb() {}
