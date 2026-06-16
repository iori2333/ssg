/*                                                                           */
/*   MaidTama.cpp   メイドさんなショットの処理                               */
/*                                                                           */
/*                                                                           */

#include "GIAN.h"
#include "MAIDTAMA.h"
#include "player_manager.h"
#include "game/cast.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "platform/graphics_backend.h"

///// [ひみつの関数] /////
static void MTamaSet(void);

static void SetT_A0(void); // めいどたまＴＹＰＥ－Ａ
static void SetT_A1(void);
static void SetT_A2(void);
static void SetT_A3(void);
static void SetT_A4(void);
static void SetT_A5(void);
static void SetT_A6(void);
static void SetT_A7(void);
static void SetT_A8(void);

static void SetT_B0(void); // めいどたまＴＹＰＥ－Ｂ
static void SetT_B1(void);
static void SetT_B2(void);
static void SetT_B3(void);
static void SetT_B4(void);
static void SetT_B5(void);
static void SetT_B6(void);
static void SetT_B7(void);
static void SetT_B8(void);

static void SetT_C0(void); // めいどたまＴＹＰＥ－Ｃ
static void SetT_C1(void);
static void SetT_C2(void);
static void SetT_C3(void);
static void SetT_C4(void);
static void SetT_C5(void);
static void SetT_C6(void);
static void SetT_C7(void);
static void SetT_C8(void);

static void SetT_D0(void); // めいどたまＴＹＰＥ－Ｄ
static void SetT_D1(void);
static void SetT_D2(void);
static void SetT_D3(void);
static void SetT_D4(void);
static void SetT_D5(void);
static void SetT_D6(void);
static void SetT_D7(void);
static void SetT_D8(void);

static void SetWideBomb(void);
static void SetHomingBomb(void);
static void SetLaserBomb(void);
static void SetCactusBomb(void);

// this->maid_tama[], this->maid_tama_ind[], this->maid_tama_now → player_manager.cpp の PlayerManager に移動

constexpr uint8_t TogeDamage[(4 * 2) + 2] = {
    // MainWeapon		// SubWeapon
    TDM_WIDE_MAIN,
    TDM_WIDE_SUB, // TYPE_A(WIDE)
    TDM_HOMING_MAIN,
    TDM_HOMING_SUB, // TYPE_B(HOMING)
    TDM_LASER_MAIN,
    TDM_LASER_SUB, // TYPE_C
    1,
    1 // ホーミングボム用
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

static void (*MaidBombFunc[4])(void) = {SetWideBomb, SetHomingBomb,
                                        SetLaserBomb, SetCactusBomb};

static constexpr auto WIDE_BOMB_TIME = (60 * 4);
static constexpr auto HOMING_BOMB_TIME = (60 * 3);
static constexpr auto LASER_BOMB_TIME = (60 * 2);
static constexpr auto CACTUS_BOMB_TIME = 0;

static constexpr uint8_t MaidBombTime[4] = {WIDE_BOMB_TIME, HOMING_BOMB_TIME,
                                            LASER_BOMB_TIME, CACTUS_BOMB_TIME};

static constexpr auto MAID_TAMA_START = 18; // 12
static constexpr auto MAID_MAIN_SHOT = 6; // 4
static constexpr auto MAID_SUB_SHOT = 9; // 6

// たま発射！！ //
void PlayerManager::SetMaidShot(void) {
  // この関数では、前回の発射状態 (Viv_St) を参照して、発射可能であるならば //
  // 発射し、そうでなければ、単にリターンする。                             //
  // なお、弾のセットには TAMA.cpp 内の関数と互換のものを使用する           //

  if ((Key_Data & KEY_TAMA) && Players.viv.toge_time == 0 &&
      Players.viv.muteki < MAID_MOVE_DISABLE_TIME) {
    Players.viv.toge_time = MAID_TAMA_START;
  }

  // ボムの発動条件を満たしていれば、発動! //
  if ((Key_Data & KEY_BOMB) && (Players.viv.bomb_time == 0) &&
      (Players.viv.muteki < VIVDEAD_VAL) && // 死亡中は Bomb を発動しない
      Players.viv.bomb && (Scroller.scene.MsgFlag == false)) {
    // if(Players.viv.weapon == 0) EnterBombPalette();

    Players.viv.bomb_time = MaidBombTime[Players.viv.weapon & 3]; // 装備ごとに変更せよ
    Players.viv.muteki = BOMBMUTEKI_VAL;
    Players.viv.bomb--;
    Players.viv.bomb_used++;
    Ranking.Add(-25); // 難易度ダウン
  }

  if (Players.viv.bomb_time) {
    Players.viv.bomb_time--;
    MaidBombFunc[Players.viv.weapon]();

    // if(Players.viv.bomb_time == 0 && Players.viv.weapon == 0)
    //	LeaveBombPalette();
  }

  if (Players.viv.toge_time) {
    MaidTamaFunc[Players.viv.weapon & 3][(Players.viv.exp + 1) >> 5]();
    Players.viv.toge_time--;
  }

  // レーザーを装備している場合 //
  if (Players.viv.weapon == 2 && Players.viv.lay_time) {
    Players.viv.lay_time--;
    if (Players.viv.lay_time < 64)
      Players.viv.lay_grp = 0;
    else if (Players.viv.lay_time < 64 + 50)
      Players.viv.lay_grp = 1;
    else if (Players.viv.lay_time < 64 + 100)
      Players.viv.lay_grp = 2;
    else if (Players.viv.lay_time < 64 + 150)
      Players.viv.lay_grp = 3;
    else
      Players.viv.lay_grp = 4;
    // Players.viv.lay_grp = (Players.viv.lay_time+63)>>6;
  }
}

// 弾移動＆ヒットチェック //
void PlayerManager::MoveMaidShot(void) {
  // この関数では、TAMA.cpp の敵弾の移動処理を使用する。もちろん、         //
  // 当たり判定については、敵に対してのものとする事！！                    //
  // 当たり判定は、この弾の座標を与えることで ENEMY.cpp 内の関数が判別して //
  // 敵に当たっているかをチェックするものとする。                          //

  int i;

  for (i = 0; i < this->maid_tama_now; i++) {
    auto *t = &this->maid_tama[this->maid_tama_ind[i]];
    if (t->c == TID_HOMING_BOMB_B) {
      Enemies.DamageAt(t->x, t->y, TogeDamage[t->c]);
      t->count++;
      if (t->count >= 19)
        t->flag = TF_DELETE;
      continue;
    }
    if (t->effect == TE_NONE) {
      Bullets.MoveByType(t);
      Bullets.MoveByOption(t);
      t->count++;
      if (((t->flag & TF_CLIP) == 0) && ((t->x) < GX_MIN || (t->x) > GX_MAX ||
                                         (t->y) < GY_MIN || (t->y) > GY_MAX))
        t->flag = TF_DELETE;

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
        fragment_set(t->x, t->y, FRG_HIT);
      }
    } else
      Bullets.MoveByEffect(t);
  }
  Indsort(Players.maid_tama_ind, this->maid_tama_now, Players.maid_tama,
          [](const TAMA_DATA &t) { return (t.flag & TF_DELETE); });

  // レーザーの当たり判定 //
  if (Players.viv.weapon == 2 && Players.viv.lay_grp) {
    // x = (Players.viv.opx>>6)+4 -8 + SBOPT_DX;
    // y = (Players.viv.opy>>6)-20;
    Enemies.DamageAt2(Players.viv.opx + (SBOPT_DX << 6), Players.viv.opy, Players.viv.lay_grp / 3 + 1);
    Enemies.DamageAt2(Players.viv.opx - (SBOPT_DX << 6), Players.viv.opy, Players.viv.lay_grp / 3 + 1);
  }
}

// ナニな弾描画 //
void PlayerManager::DrawMaidShot(void) {
  // ここでは、さすがにTAMA.cpp 内の関数を使用するわけにはいかないので、 //
  // 独自に描画ルーチンを展開する。                                      //

  int i, x, y;
  PIXEL_LTRB src, ltemp;
  static PIXEL_LTRB HomingBomb[5] = {{520, 104, 520 + 8, 104 + 8},
                                     {528, 104, 528 + 16, 104 + 16},
                                     {544, 104, 544 + 24, 104 + 24},
                                     {568, 104, 568 + 32, 104 + 32},
                                     {600, 104, 600 + 40, 104 + 40}};

  for (i = 0; i < this->maid_tama_now; i++) {
    auto *t = &this->maid_tama[this->maid_tama_ind[i]];

    x = (t->x >> 6) - 8; // -8 は座標の補正用です
    y = (t->y >> 6) - 8; // 上に同じ

    // 弾の種類により、描画指定用矩形をセットする //
    switch (t->c) {
    case (TID_WIDE_MAIN):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 176, 16, 16};
      break;
    case (TID_WIDE_SUB):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 192, 16, 16};
      break;
    case (TID_HOMING_MAIN):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 208, 16, 16};
      break;
    case (TID_HOMING_SUB):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 224, 16, 16};
      break;
    case (TID_HOMING_BOMB_A):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 288, 16, 16};
      break;
    case (TID_LASER_SUB):
      src = PIXEL_LTWH{(384 + ((t->d + 8) & 0xf0)), 256, 16, 16};
      break;

    case (TID_HOMING_BOMB_B):
      src = HomingBomb[(t->count / 4) % 5];
      break;
    }

    // 完全判定付きクリッピング //
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, src);
  }

  // レーザーの描画 //
  if (Players.viv.weapon == 2 && Players.viv.lay_grp) {
    ltemp = PIXEL_LTWH{(384 + ((Players.viv.lay_grp - 1) << 4)), 240, 8, 16};

    x = (Players.viv.opx >> 6) + 4 - 8 + SBOPT_DX;
    y = (Players.viv.opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    x = (Players.viv.opx >> 6) + 4 - 8 - SBOPT_DX;
    y = (Players.viv.opy >> 6) - 20;
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);

    ltemp = PIXEL_LTWH{(384 + 8 + ((Players.viv.lay_grp - 1) << 4)), 240, 8, 16};
    for (i = (Players.viv.opy >> 6) - 36; i > -16; i -= 16) {
      x = (Players.viv.opx >> 6) + 4 - 8 + SBOPT_DX;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
    for (i = (Players.viv.opy >> 6) - 36; i > -16; i -= 16) {
      x = (Players.viv.opx >> 6) + 4 - 8 - SBOPT_DX;
      y = i;
      GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, ltemp);
    }
  }
}

// 弾ハッシュテーブル初期化 //
void PlayerManager::SetMaidShotIndices(void) {
  int i;

  // この配列を初期化することで全ての弾を初期化する事になる //
  for (i = 0; i < MAIDTAMA_MAX; i++) {
    this->maid_tama_ind[i] = i;
    // memset(Players.maid_tama+i,0,sizeof(TAMA_DATA));
  }

  // 現在の個数を０初期化するのを忘れずに //
  this->maid_tama_now = 0;
}

static void MTamaSet(void) {
  for (decltype(Bullets.command.n) i = 0; i < Bullets.command.n; i++) {
    if (Players.maid_tama_now + 1 >= MAIDTAMA_MAX)
      return; // セットできない場合

    auto *t = &Players.maid_tama[Players.maid_tama_ind[Players.maid_tama_now++]]; // 弾ポインタをセット

    t->x = t->tx = Bullets.command.x; // X座標のセット
    t->y = t->ty = Bullets.command.y; // Y座標のセット

    t->v = t->v0 = Bullets.Speed(i); // 初速度のセット
    t->a = Bullets.command.a;             // 注意：サイズは char

    t->d = Bullets.Dir(i);   // 弾の発射角度
    t->d16 = (t->d << 8); // 角速度のある運動で使用

    t->vx = cosl(t->d, t->v); // 速度のＸ成分セット
    t->vy = sinl(t->d, t->v); // 速度のＹ成分セット

    t->vd = Bullets.command.vd;         // 角速度もしくはホーミング率
    t->c = Bullets.command.c;           // 弾のＩＤ
    t->rep = Bullets.command.rep;       // 繰り返し回数
    t->type = Bullets.command.type;     // 弾の種類
    t->option = Bullets.command.option; // 弾の属性(バイブ、反射等)
    t->effect = 0;              // Bullets.command.cmd & 0xf0;			//
                                // 弾のエフェクト
    t->count = 0;               // カウンタの初期化
    t->flag = Bullets.Flag();      // フラグの初期化
  }
}

inline bool IsMainShot(uint16_t t) {
  return (t == MAID_MAIN_SHOT || t == MAID_MAIN_SHOT * 2 || t == MAID_MAIN_SHOT * 3);
}
inline bool IsSubShot(uint16_t t) {
  return (t == 0 || t == MAID_SUB_SHOT) && Players.viv.bomb_time == 0;
}

void PlayerManager::SetMLaser(uint16_t time) {
  if (Players.viv.bomb_time || Players.viv.muteki > MAID_MOVE_DISABLE_TIME) {
    Players.viv.lay_time = 0;
    Players.viv.lay_grp = 0;
    return;
  }

  if (Players.viv.lay_time == 0) {
    Players.viv.lay_time = time;
    Snd_SEPlay(SOUND_ID_SBLASER, Players.viv.x);
  }
}

// ショットＴＹＰＥ－Ａ //
static void SetT_A0(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット単発のみ //
    TamaSTDForm(TID_WIDE_MAIN);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_A1(void) {
  char dd;

  if (IsSubShot(Players.viv.toge_time)) {
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 + 5, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }

  if (IsMainShot(Players.viv.toge_time)) {
    // 軽く振り分けるメインショット //
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

static void SetT_A2(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット２連 //
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
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    TamaSetDeg(-64 + 5, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }
}

static void SetT_A3(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット３ＷＡＹ //
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
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 + 5, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 5, 0);
    MTamaSet();
  }
}

static void SetT_A4(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット３ＷＡＹ //
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
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 + 8, 7); //(-64+5,7);
    TamaSetSpd(54, 0);
    TamaSetNum(2, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 8, 7); //(-64-5,7);
    MTamaSet();
  }
}

static void SetT_A5(void) { SetT_A4(); }

static void SetT_A6(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット４ＷＡＹ //
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
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 + 10, 8); //-64+6,4);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 10, 8); //(-64-6,4);
    MTamaSet();
  }
}

static void SetT_A7(void) { SetT_A6(); }

static void SetT_A8(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット４ＷＡＹ //
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
    // オプションのショット(右) //
    TamaSTDForm(TID_WIDE_SUB);
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 + 12, 8); //(-64+7,4);
    TamaSetSpd(54, 0);
    TamaSetNum(4, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(-64 - 12, 8); //(-64-7,4);
    MTamaSet();
  }
}

// ショットＴＹＰＥ－Ｂ //
static void SetT_B0(void) {
  char dd;

  if (IsMainShot(Players.viv.toge_time)) {
    // 軽く振り分けるメインショット //
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

static void SetT_B1(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット２連 //
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
    // ホーミング弾 //
    // オプションのショット(右) //
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B2(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット３ＷＡＹ //
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
    // ホーミング弾 //
    // オプションのショット(右) //
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B3(void) { SetT_B2(); }

static void SetT_B4(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット５ＷＡＹ //
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
    // ホーミング弾 //
    // オプションのショット(右) //
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B5(void) { SetT_B4(); }

static void SetT_B6(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット５ＷＡＹ //
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
    // ホーミング弾 //
    // オプションのショット(右) //
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 5, 0);
    TamaSetNum(1, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(64 + 5, 0);
    MTamaSet();
  }
}

static void SetT_B7(void) { SetT_B6(); }

static void SetT_B8(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット５ＷＡＹ //
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
    // ホーミング弾 //
    // オプションのショット(右) //
    TamaSTDForm(TID_HOMING_SUB);
    Bullets.command.type = T_SBHOMING;
    Bullets.command.rep = 64;
    Bullets.command.vd = 5;
    TamaSetXY(Players.viv.opx + SBOPT_DX * 64, Players.viv.opy);
    TamaSetSpd(28, 4);
    TamaSetDeg(64 - 22, 30);
    TamaSetNum(2, 0);
    MTamaSet();

    // オプションのショット(左) //
    TamaSetXY(Players.viv.opx - SBOPT_DX * 64, Players.viv.opy);
    TamaSetDeg(64 + 22, 30);
    MTamaSet();
  }
}

// ショットＴＹＰＥ－Ｃ //
static void SetT_C0(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット単発のみ //
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
  }
}

static void SetT_C1(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央に２列ショット //
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x - (6 * 64), Players.viv.y);
    TamaSetDeg(-64, 0);
    TamaSetSpd(54, 0);
    TamaSetNum(1, 0);
    MTamaSet();
    Bullets.command.x += (12 * 64);
    MTamaSet();
  }

  Players.SetMLaser(64 + 50);
}

static void SetT_C2(void) { SetT_C1(); }

static void SetT_C3(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット３ＷＡＹ //
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(3, 0);
    MTamaSet();
  }

  Players.SetMLaser(64 + 100);
}

static void SetT_C4(void) { SetT_C3(); }

static void SetT_C5(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット４ＷＡＹ(中央は２列で) //
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

  Players.SetMLaser(64 + 150);
}

static void SetT_C6(void) { SetT_C5(); }

static void SetT_C7(void) { SetT_C5(); }

static void SetT_C8(void) {
  if (IsMainShot(Players.viv.toge_time)) {
    // 中央にショット５ＷＡＹ //
    TamaSTDForm(TID_LASER_SUB);
    TamaSetXY(Players.viv.x, Players.viv.y);
    TamaSetDeg(-64, 6);
    TamaSetSpd(54, 0);
    TamaSetNum(5, 0);
    MTamaSet();
  }

  Players.SetMLaser(64 + 200);
}

// ショットＴＹＰＥ－Ｄ //
static void SetT_D0(void) {}

static void SetT_D1(void) {}

static void SetT_D2(void) {}

static void SetT_D3(void) {}

static void SetT_D4(void) {}

static void SetT_D5(void) {}

static void SetT_D6(void) {}

static void SetT_D7(void) {}

static void SetT_D8(void) {}

static void SetWideBomb(void) {
  int dx, dy, l;

  if (Players.viv.bomb_time > WIDE_BOMB_TIME - 30)
    return;

  const uint8_t d = Cast::down<uint8_t>(Players.viv.bomb_time * 3u);
  l = (WIDE_BOMB_TIME - Players.viv.bomb_time) * 26; // 16-32
  dx = GX_MID + 64 * 70 / 2 + cosl(d, l << 1);
  dy = GY_MID - 64 * 90 / 2 + sinl(d << 1, l);

  fragment_set(dx, dy, FRG_STAR1);
  fragment_set(dx, dy, FRG_STAR1);
  fragment_set(dx, dy, FRG_STAR2);

  Enemies.DamageAll(1);
}

static void SetHomingBomb(void) {
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

    // 欠陥があるので、廃止 //
    // ObjectLockOn(&HomingX, &HomingY, 32*64, 32*64);
  }
}

// こいつは、Set というよりも、 HitCheck 的な役割を果たす //
static void SetLaserBomb(void) {
  int ox, oy;
  int i;

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

static void SetCactusBomb(void) {}
