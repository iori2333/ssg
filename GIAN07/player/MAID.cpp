/*                                                                           */
/*   Maid.cpp   メイドさん関連の処理                                         */
/*                                                                           */
/*                                                                           */

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <inttypes.h> // for PRId64

#include "config.h"
#include "FONTUTY.h"
#include "GEOMETRY.h"
#include "gian.h"
#include "MAID.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"

// Viv → player_manager.cpp の PlayerManager に移動

// --- Player メソッド実装 ---

void Player::DrawWideBomb() {
  static PIXEL_LTRB data[6] = {
      {0, 0, 210, 240},           {210, 0, 210 * 2, 240},
      {210 * 2, 0, 210 * 3, 240}, {0, 240, 210, 480},
      {210, 240, 210 * 2, 480},   {210 * 2, 240, 210 * 3, 480}};

  int x, y;
  int t;

  constexpr int BX_MIN = (X_MIN + 100);
  constexpr int BY_MIN = (Y_MIN + 100);

  if (weapon != 0 || bomb_time == 0)
    return;

  x = BX_MIN;
  y = BY_MIN;

  if (bomb_time > 80) {
    t = ((60 * 4 - bomb_time) / 4);
    if (t < 0)
      t = 0;
    if (t > 5)
      t = 5;
  } else {
    t = (bomb_time / 4);
    if (t > 5)
      t = 5;
  }

  GrpSurface_Blit({x, y}, SURFACE_ID::BOMBER, data[t]);
}

void Player::DrawLaserBomb() {
  constexpr RGBA col_channeled = RGB216{0, 0, 5}.ToRGB().WithAlpha(0xFF);
  VERTEX_XY p[4];
  int i, w;
  int lx, ly;
  int wx, wy;
  const auto LaserDeg = GetLaserDeg();

  GrpGeom->Lock();
  if (LaserDeg < 58) {
    for (w = 3; w > 0; w--) {
      for (i = -3; i <= 3; i++) {
        const auto d = GetRightLaserDeg(LaserDeg, i);

        lx = cosl(d, 850);
        ly = sinl(d, 850);
        wx = cosl(d + 64, w);
        wy = sinl(d + 64, w);
        p[0].x = (opx >> 6) + SBOPT_DX + 0 + wx;
        p[0].y = (opy >> 6) + 0 + wy;
        p[3].x = (opx >> 6) + SBOPT_DX + 0 - wx;
        p[3].y = (opy >> 6) + 0 - wy;
        p[2].x = (opx >> 6) + SBOPT_DX + lx - wx;
        p[2].y = (opy >> 6) + ly - wy;
        p[1].x = (opx >> 6) + SBOPT_DX + lx + wx;
        p[1].y = (opy >> 6) + ly + wy;
        if (auto *gp = GrpGeom_Poly()) {
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, p, col_channeled);
        } else if (auto *gf = GrpGeom_FB()) {
          switch (w) {
          case (1): gf->SetColor({4, 4, 5}); break;
          case (2): gf->SetColor({2, 2, 5}); break;
          case (3): gf->SetColor({0, 0, 5}); break;
          }
          gf->DrawTriangleFan(p);
        }
      }
      for (i = -3; i <= 3; i++) {
        const auto d = GetLeftLaserDeg(LaserDeg, i);

        lx = cosl(d, 850);
        ly = sinl(d, 850);
        wx = cosl(d + 64, w);
        wy = sinl(d + 64, w);
        p[0].x = (opx >> 6) - SBOPT_DX + 0 + wx;
        p[0].y = (opy >> 6) + 0 + wy;
        p[3].x = (opx >> 6) - SBOPT_DX + 0 - wx;
        p[3].y = (opy >> 6) + 0 - wy;
        p[2].x = (opx >> 6) - SBOPT_DX + lx - wx;
        p[2].y = (opy >> 6) + ly - wy;
        p[1].x = (opx >> 6) - SBOPT_DX + lx + wx;
        p[1].y = (opy >> 6) + ly + wy;
        if (auto *gp = GrpGeom_Poly()) {
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, p, col_channeled);
        } else if (auto *gf = GrpGeom_FB()) {
          switch (w) {
          case (1): gf->SetColor({4, 4, 5}); break;
          case (2): gf->SetColor({2, 2, 5}); break;
          case (3): gf->SetColor({0, 0, 5}); break;
          }
          gf->DrawTriangleFan(p);
        }
      }
      if (GrpGeom_Poly()) {
        break;
      }
    }
  } else if (LaserDeg < 150) {
    uint8_t c = 0;
    for (w = 12 - (LaserDeg - 64) / 8; w > 0; w -= 2, c++) {
      for (i = -3; i <= 3; i++) {
        const auto d = GetRightLaserDeg(LaserDeg, i);

        lx = cosl(d, 850);
        ly = sinl(d, 850);
        wx = cosl(d + 64, w);
        wy = sinl(d + 64, w);
        p[0].x = (opx >> 6) + SBOPT_DX + 0 + wx;
        p[0].y = (opy >> 6) + 0 + wy;
        p[3].x = (opx >> 6) + SBOPT_DX + 0 - wx;
        p[3].y = (opy >> 6) + 0 - wy;
        p[2].x = (opx >> 6) + SBOPT_DX + lx - wx;
        p[2].y = (opy >> 6) + ly - wy;
        p[1].x = (opx >> 6) + SBOPT_DX + lx + wx;
        p[1].y = (opy >> 6) + ly + wy;
        if (auto *gp = GrpGeom_Poly()) {
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, p, col_channeled);
        } else if (auto *gf = GrpGeom_FB()) {
          gf->SetColor({c, c, 5u});
          gf->DrawTriangleFan(p);
        }
      }
      for (i = -3; i <= 3; i++) {
        const auto d = GetLeftLaserDeg(LaserDeg, i);

        lx = cosl(d, 850);
        ly = sinl(d, 850);
        wx = cosl(d + 64, w);
        wy = sinl(d + 64, w);
        p[0].x = (opx >> 6) - SBOPT_DX + 0 + wx;
        p[0].y = (opy >> 6) + 0 + wy;
        p[3].x = (opx >> 6) - SBOPT_DX + 0 - wx;
        p[3].y = (opy >> 6) + 0 - wy;
        p[2].x = (opx >> 6) - SBOPT_DX + lx - wx;
        p[2].y = (opy >> 6) + ly - wy;
        p[1].x = (opx >> 6) - SBOPT_DX + lx + wx;
        p[1].y = (opy >> 6) + ly + wy;
        if (auto *gp = GrpGeom_Poly()) {
          gp->SetAlphaOne();
          GeomGrdRectA(*gp, p, col_channeled);
        } else if (auto *gf = GrpGeom_FB()) {
          gf->SetColor({c, c, 5u});
          gf->DrawTriangleFan(p);
        }
      }
      if (GrpGeom_Poly()) {
        break;
      }
    }
  }

  GrpGeom->Unlock();
}

void Player::Draw() {
  static PIXEL_LTRB VivBit[4][2] = {
      {{480, 128, 480 + 24, 128 + 24},
       {504, 128, 504 + 24, 128 + 24}}, // ワイド
      {{480, 152, 480 + 24, 152 + 24},
       {504, 152, 504 + 24, 152 + 24}}, // ホーミング
      {{528, 152, 528 + 24, 152 + 24},
       {552, 152, 552 + 24, 152 + 24}}, // レーザー
      {{480, 152, 480 + 24, 152 + 24}, {504, 152, 504 + 24, 152 + 24}}, // 仮
  };

  static uint8_t draw_flag = 0;
  static uint8_t draw_flag2 = 0;

  const auto sx = ((x >> 6) - 16);
  const auto sy = ((y >> 6) - 24);
  const auto ox = ((opx >> 6) - 12);
  const auto oy = ((opy >> 6) - 12);
  PIXEL_LTRB src;

  draw_flag = 1 - draw_flag;
  draw_flag2++;

  if (muteki == VIVDEAD_VAL)
    draw_flag = 0;

  if (muteki == 0 || draw_flag) {
    src = PIXEL_LTWH{(384 + (GrpID * 32)), 128, (16 * 2), (16 * 3)};
    GrpSurface_Blit({sx, sy}, SURFACE_ID::SYSTEM, src);
  }

  if (((exp + 1) >> 5)) {
    if (muteki < VIVDEAD_VAL) {
      src = VivBit[weapon & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox + SBOPT_DX), oy}, SURFACE_ID::SYSTEM, src);
      src = VivBit[weapon & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox - SBOPT_DX), oy}, SURFACE_ID::SYSTEM, src);
    }
  }

  if (bomb_time && weapon == 2)
    DrawLaserBomb();
}

void Player::DrawStatus() {
  int i, temp;
  constexpr PIXEL_LTRB src = {0, 80, 128, (80 + 24)};
  char buf[100];

  if (evade_c) {
    GrpGeom->Lock();
    GrpGeom->SetColor({5, 1, 0});
    GrpGeom->SetAlphaOne();
    for (i = 0; i <= 10; i++) {
      temp = 128 + 9 + (evade_c >> 2) + (5 - i);
      if (temp > (128 + 8)) {
        GrpGeom->DrawBoxA((128 + 8), (16 + 3 + i), temp, (16 + 3 + i + 1));
      }
    }
    GrpGeom->Unlock();
  }
  GrpSurface_Blit({128, 16}, SURFACE_ID::SYSTEM, src);
  sprintf(buf, "%3d", evade);
  GrpPut57(128 + 95, 16 + 91 - 80, buf);

  sprintf(buf, "%9" PRId64, score);
  GrpPut16(128, 0, buf);

  sprintf(buf, "       Bomb %1d", bomb);
  GrpPut16(280, 0, buf);

  for (i = 0; i < left; i++) {
    constexpr PIXEL_LTWH life_src = {608, 432, 16, 16};
    GrpSurface_Blit({(280 + (i * 14)), 0}, SURFACE_ID::SYSTEM, life_src);
  }
}

void Player::Update() {
  int vx, vy, v;
  constexpr int VivSpeed = (64 * 18);
  char buf[100];

  // かすり残り時間を減らす //
  if (evade_c) {
    if (bomb_time && evade_c >= 2)
      evade_c -= 2;
    else
      evade_c -= 1;

    if (evade_c == 0) {
      sprintf(buf, "%3d Evade  %7dPts", evade, evadesc);
      Effects.SpawnStringEffect(180, 40, buf);
      AddScore(evadesc);
      evade = 0;
      evadesc = 0;
    }
  }

  // 無敵時間を減らす(ボム中は減らさない) //
  if (muteki && bomb_time == 0)
    muteki--;

  // 得点変化処理 //
  if (dscore >= 100000)
    score += 100000, dscore -= 100000;
  else if (dscore >= 20000)
    score += 20000, dscore -= 20000;
  else if (dscore >= 2000)
    score += 2000, dscore -= 2000;
  else if (dscore >= 200)
    score += 200, dscore -= 200;
  else if (dscore >= 20)
    score += 20, dscore -= 20;
  else if (dscore >= 10)
    score += 10, dscore -= 10;

  // 押しっぱなし減速を有効にするのか //
  if (ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) {
    if (Key_Data & KEY_TAMA) {
      if (ShiftCounter < 8)
        ShiftCounter++;
      else
        Key_Data = Key_Data | KEY_SHIFT;
    } else {
      ShiftCounter = 0;
    }
  }

  if (muteki < MAID_MOVE_DISABLE_TIME) {
    vx = vy = 0;
    v = (Key_Data & KEY_SHIFT) ? (VivSpeed / 3) : VivSpeed;
    if (Key_Data & KEY_UP)
      vy -= v;
    if (Key_Data & KEY_DOWN)
      vy += v;
    if (Key_Data & KEY_LEFT)
      vx -= v;
    if (Key_Data & KEY_RIGHT)
      vx += v;

    if (vx && vy) {
      x += (vx / 6);
      y += (vy / 6);
    } else {
      x += (vx >> 2);
      y += (vy >> 2);
    }

    if (y < SY_MIN)
      y = SY_MIN;
    else if (y > SY_MAX)
      y = SY_MAX;

    if (x < SX_MIN)
      x = SX_MIN;
    else if (x > SX_MAX)
      x = SX_MAX;
  } else {
    vx = 0;
    vy = -(64 + 32);
    y += vy;
  }

  if (vx > 0)
    GrpID = 2;
  else if (vx < 0)
    GrpID = 0;
  else
    GrpID = 1;

  opx = x;
  opy = y;

  // オプションの処理 //
  if (vx < 0)
    vx += 64;
  if (vx > 0)
    vx -= 64;
  if (vy < 0)
    vy += 64;
  if (vy > 0)
    vy -= 64;

  if (vx < 0 && vx < 6 * 64)
    vx += 2 * 64;
  if (vx > 0 && vx > -6 * 64)
    vx -= 2 * 64;
  if (vy < 0 && vy < 10 * 64)
    vy += 2 * 64;
  if (vy > 0 && vy > -10 * 64)
    vy -= 2 * 64;

  opx = x + vx;
  opy = y + vy + 64 * 6;

  // 弾＆ボムのセット //
  Players.SetMaidShot();

  if (bomb_time) {
    Bullets.Clear();
    Lasers.Clear();
  }

  BuzzSound = false;
}

void Player::Initialize() {
  PrepareNextStage();

  score = 0;
  dscore = 0;
  exp = 0;
  exp2 = 0;
  bomb = ConfigDat.BombStock.v;
  left = ConfigDat.PlayerStock.v;
  credit = 4;

  miss_count = 0;
  bomb_used = 0;

  bomb_time = 0;
  evade_c = evade = 0;
  evadesc = 0;
  evade_sum = 0;

  GrpID = 1;

  muteki = VIVDEAD_VAL;

  bGameOver = false;

  toge_ex = 0;
  toge_time = 0;
  lay_time = 0;
  lay_grp = 0;
  ShiftCounter = 0;

  BuzzSound = false;
}

void Player::PrepareNextStage() {
  x = opx = SX_START;
  y = opx = SY_START;
  vx = 0;
  vy = 0;

  toge_ex = 0;
  toge_time = 0;
  lay_time = 0;
  lay_grp = 0;

  muteki = VIVDEAD_VAL;
  bomb_time = 0;
  ShiftCounter = 0;

  BuzzSound = false;
}

void Player::OnDeath() {
  int i;

#ifdef PBG_DEBUG
  if (!DebugDat.Hit)
    return;
#endif

  if (ConfigDat.PracticeMode.v == PRACTICE_INVINCIBLE) {
    Effects.SpawnFragment(x, y, FRG_FATCIRCLE);
    for (i = 0; i < 50; i++)
      Effects.SpawnFragment(x, y, FRG_HEART);
    Snd_SEPlay(SOUND_ID_DEAD);
    muteki = 30;
    return;
  }

  // 自動ボム：练习模式为自动Bomb以上时，Bomb キーが押されておらず、かつ Bomb
  // 残量がある場合、 死亡の代わりに自動で Bomb を発動する
  if (ConfigDat.PracticeMode.v == PRACTICE_AUTOBOMB && !(Key_Data & KEY_BOMB) &&
      (bomb_time == 0) && (bomb > 0) && (Scroller.scene.MsgFlag == false)) {
    static constexpr uint8_t bomb_time_tbl[4] = {60 * 4, 60 * 3, 60 * 2, 0};
    bomb_time = bomb_time_tbl[weapon & 3];
    muteki = BOMBMUTEKI_VAL;
    bomb--;
    bomb_used++;
    Ranking.Add(-25); // 自动Bomb降低Rank
    Bullets.Clear();
    Lasers.Clear();
    return;
  }

  Effects.SpawnFragment(x, y, FRG_FATCIRCLE);

  for (i = 0; i < 50; i++)
    Effects.SpawnFragment(x, y, FRG_HEART);

  Snd_SEPlay(SOUND_ID_DEAD);

  x = opx = SX_START;
  y = opx = SY_START;
  vx = 0;
  vy = 0;

  lay_time = 0;
  lay_grp = 0;

  bomb = ConfigDat.BombStock.v;
  muteki = VIVDEAD_VAL;

  Ranking.Add(-100); // 死亡降低Rank

  if (left) {
    left -= 1;
    miss_count++;
  } else {
    bGameOver = true;
    score += dscore;

    evade_c = 0;
    evade = 0;
    evadesc = 0;

    GameOverInit();
  }

  Bullets.Clear();
  Lasers.Clear();
}

void Player::AddEvade(uint8_t n) {
  AddEvadeEx(x, y, n);
}

void Player::AddEvadeEx(int ex, int ey, uint8_t n) {
  int i;

  if (n) {
    if (BuzzSound == false) {
      Snd_SEPlay(SOUND_ID_BUZZ, ex);
      BuzzSound = true;
    }
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
  }

  for (i = 0; i < n; i++) {
    if (evade == 999) {
      evade_c = 1;
      return;
    }
    evade += 1;
    evade_sum += 1;
    evadesc += evade * 20;
  }

  if (evade)
    evade_c = EVADETIME_MAX;
}

void Player::AddScore(int sc) {
  dscore += sc;
}

void Player::PowerUp(uint8_t damage) {
  exp2 += damage;

  switch ((Cast::up<uint16_t>(exp) + 1) >> 5) {
  case (0):
    if (exp2 > 5 - 3)
      exp++, exp2 = 0;
    return;
  case (1):
    if (exp2 > 25 - 15)
      exp++, exp2 = 0;
    return;
  case (2):
    if (exp2 > 50 - 20)
      exp++, exp2 = 0;
    return;
  case (3):
    if (exp2 > 80)
      exp++, exp2 = 0;
    return;
  case (4):
    if (exp2 > 120)
      exp++, exp2 = 0;
    return;
  case (5):
    if (exp2 > 140)
      exp++, exp2 = 0;
    return;
  case (6):
    if (exp2 > 160)
      exp++, exp2 = 0;
    return;
  case (7):
    if (exp2 > 180)
      exp++, exp2 = 0;
    return;
  case (8):
    return; // フルパワーアップ時
  }
}

uint8_t Player::GetLaserDeg() {
  return ((120 - bomb_time) * 3) / 2;
}

#pragma warning(suppress : 26497) // f.4
uint8_t Player::GetLeftOrRightLaserDeg(uint8_t LaserDeg, int i) {
  return ((LaserDeg < 58)
              ? ((LaserDeg * 3) + ((i * (64 - LaserDeg)) / 2))
              : ((58 * 3) + ((i * (64 - (std::min)(62, int{LaserDeg}))) / 2)));
}

uint8_t Player::GetRightLaserDeg(uint8_t LaserDeg, int i) {
  return (64 + 48 - GetLeftOrRightLaserDeg(LaserDeg, i));
}

uint8_t Player::GetLeftLaserDeg(uint8_t LaserDeg, int i) {
  return (64 - 48 + GetLeftOrRightLaserDeg(LaserDeg, i));
}

// 後方互換用：MAIDTAMA.cpp などから参照される自由関数ラッパー
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i) {
  return Players.viv.GetRightLaserDeg(LaserDeg, i);
}
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i) {
  return Players.viv.GetLeftLaserDeg(LaserDeg, i);
}
