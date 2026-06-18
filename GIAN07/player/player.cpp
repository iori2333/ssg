///
/// Player - Player-related processing
///

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include <cinttypes> // for PRId64

#include <algorithm>

#include <format>
#include <utility>

#include "config.h"
#include "font_uty.h"
#include "game/input.h"
#include "game/snd.h"
#include "game/ut_math.h"
#include "geometry.h"
#include "gian.h"
#include "player.h"

// Moved to PlayerManager in player_manager.cpp

// --- Player method implementations ---

void Player::DrawWideBomb() const {
  static PIXEL_LTRB data[6] = {
      {0, 0, 210, 240},           {210, 0, 210 * 2, 240},
      {210 * 2, 0, 210 * 3, 240}, {0, 240, 210, 480},
      {210, 240, 210 * 2, 480},   {210 * 2, 240, 210 * 3, 480}};

  int x = 0;
  int y = 0;
  int t = 0;

  constexpr int BX_MIN = (X_MIN + 100);
  constexpr int BY_MIN = (Y_MIN + 100);

  if (weapon != 0 || bomb_time == 0) {
    return;
  }

  x = BX_MIN;
  y = BY_MIN;

  if (bomb_time > 80) {
    t = (((60 * 4) - bomb_time) / 4);
    t = std::max(t, 0);
    t = std::min(t, 5);
  } else {
    t = (bomb_time / 4);
    t = std::min(t, 5);
  }

  GrpSurface_Blit({x, y}, SURFACE_ID::BOMBER, data[t]);
}

void Player::DrawLaserBomb() const {
  constexpr RGBA col_channeled = RGB216{0, 0, 5}.ToRGB().WithAlpha(0xFF);
  VERTEX_XY p[4];
  int i = 0;
  int w = 0;
  int lx = 0;
  int ly = 0;
  int wx = 0;
  int wy = 0;
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
          case 1:
            gf->SetColor({4, 4, 5});
            break;
          case 2:
            gf->SetColor({2, 2, 5});
            break;
          case 3:
            gf->SetColor({0, 0, 5});
            break;
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
          case 1:
            gf->SetColor({4, 4, 5});
            break;
          case 2:
            gf->SetColor({2, 2, 5});
            break;
          case 3:
            gf->SetColor({0, 0, 5});
            break;
          }
          gf->DrawTriangleFan(p);
        }
      }
      if (GrpGeom_Poly() != nullptr) {
        break;
      }
    }
  } else if (LaserDeg < 150) {
    uint8_t c = 0;
    for (w = 12 - ((LaserDeg - 64) / 8); w > 0; w -= 2, c++) {
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
          gf->SetColor({c, c, 5U});
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
          gf->SetColor({c, c, 5U});
          gf->DrawTriangleFan(p);
        }
      }
      if (GrpGeom_Poly() != nullptr) {
        break;
      }
    }
  }

  GrpGeom->Unlock();
}

void Player::Draw() {
  static PIXEL_LTRB VivBit[4][2] = {
      {{480, 128, 480 + 24, 128 + 24},
       {504, 128, 504 + 24, 128 + 24}}, // wide
      {{480, 152, 480 + 24, 152 + 24},
       {504, 152, 504 + 24, 152 + 24}}, // homing
      {{528, 152, 528 + 24, 152 + 24},
       {552, 152, 552 + 24, 152 + 24}}, // laser
      {{480, 152, 480 + 24, 152 + 24}, {504, 152, 504 + 24, 152 + 24}}, // temp
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

  if (muteki == VIVDEAD_VAL) {
    draw_flag = 0;
  }

  if (muteki == 0 || (draw_flag != 0U)) {
    src = PIXEL_LTWH{(384 + (GrpID * 32)), 128, (16 * 2), (16 * 3)};
    GrpSurface_Blit({sx, sy}, SURFACE_ID::SYSTEM, src);
  }

  if (((exp + 1) >> 5) != 0) {
    if (muteki < VIVDEAD_VAL) {
      src = VivBit[weapon & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox + SBOPT_DX), oy}, SURFACE_ID::SYSTEM, src);
      src = VivBit[weapon & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox - SBOPT_DX), oy}, SURFACE_ID::SYSTEM, src);
    }
  }

  if ((bomb_time != 0U) && weapon == 2) {
    DrawLaserBomb();
  }
}

void Player::DrawStatus() const {
  int i = 0;
  int temp = 0;
  constexpr PIXEL_LTRB src = {0, 80, 128, (80 + 24)};

  if (evade_c != 0U) {
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
  GrpPut57(128 + 95, 16 + 91 - 80, std::format("{:3}", evade).c_str());

  GrpPut16(128, 0, std::format("{:9}", score).c_str());

  GrpPut16(280, 0, std::format("       Bomb {}", bomb).c_str());

  for (i = 0; std::cmp_less(i, left); i++) {
    constexpr PIXEL_LTWH life_src = {608, 432, 16, 16};
    GrpSurface_Blit({(280 + (i * 14)), 0}, SURFACE_ID::SYSTEM, life_src);
  }
}

void Player::Update() {
  int vx = 0;
  int vy = 0;
  int v = 0;
  constexpr int VivSpeed = (64 * 18);

  // Decrease graze remaining time
  if (evade_c != 0U) {
    if ((bomb_time != 0U) && evade_c >= 2) {
      evade_c -= 2;
    } else {
      evade_c -= 1;
    }

    if (evade_c == 0) {
      Effects.SpawnStringEffect(
          180, 40,
          std::format("{:3} Evade  {:7}Pts", evade, evadesc).c_str());
      AddScore(evadesc);
      evade = 0;
      evadesc = 0;
    }
  }

  // Decrease invincibility time (not during bomb)
  if ((muteki != 0U) && bomb_time == 0) {
    muteki--;
  }

  // Deathbomb window countdown
  if (deathbomb_time != 0U) {
    deathbomb_time--;
    if (deathbomb_time == 0U) {
      OnDeath(false); // sound already played in OnHit
    }
  }

  // Score change processing
  if (dscore >= 100000) {
    score += 100000, dscore -= 100000;
  } else if (dscore >= 20000) {
    score += 20000, dscore -= 20000;
  } else if (dscore >= 2000) {
    score += 2000, dscore -= 2000;
  } else if (dscore >= 200) {
    score += 200, dscore -= 200;
  } else if (dscore >= 20) {
    score += 20, dscore -= 20;
  } else if (dscore >= 10) {
    score += 10, dscore -= 10;
  }

  // Enable held-button slowdown
  if ((ConfigDat.InputFlags.v & INPF_Z_SPDDOWN_ENABLE) != 0) {
    if ((Key_Data & KEY_TAMA) != 0) {
      if (ShiftCounter < 8) {
        ShiftCounter++;
      } else {
        Key_Data = Key_Data | KEY_SHIFT;
      }
    } else {
      ShiftCounter = 0;
    }
  }

  if (muteki < MAID_MOVE_DISABLE_TIME) {
    vx = vy = 0;
    v = ((Key_Data & KEY_SHIFT) != 0) ? (VivSpeed / 3) : VivSpeed;
    if ((Key_Data & KEY_UP) != 0) {
      vy -= v;
    }
    if ((Key_Data & KEY_DOWN) != 0) {
      vy += v;
    }
    if ((Key_Data & KEY_LEFT) != 0) {
      vx -= v;
    }
    if ((Key_Data & KEY_RIGHT) != 0) {
      vx += v;
    }

    if ((vx != 0) && (vy != 0)) {
      x += (vx / 6);
      y += (vy / 6);
    } else {
      x += (vx >> 2);
      y += (vy >> 2);
    }

    if (y < SY_MIN) {
      y = SY_MIN;
    } else if (y > SY_MAX) {
      y = SY_MAX;
    }

    if (x < SX_MIN) {
      x = SX_MIN;
    } else if (x > SX_MAX) {
      x = SX_MAX;
    }
  } else {
    vx = 0;
    vy = -(64 + 32);
    y += vy;
  }

  if (vx > 0) {
    GrpID = 2;
  } else if (vx < 0) {
    GrpID = 0;
  } else {
    GrpID = 1;
  }

  opx = x;
  opy = y;

  // Option processing
  if (vx < 0) {
    vx += 64;
  }
  if (vx > 0) {
    vx -= 64;
  }
  if (vy < 0) {
    vy += 64;
  }
  if (vy > 0) {
    vy -= 64;
  }

  if (vx < 0 && vx < 6 * 64) {
    vx += 2 * 64;
  }
  if (vx > 0 && vx > -6 * 64) {
    vx -= 2 * 64;
  }
  if (vy < 0 && vy < 10 * 64) {
    vy += 2 * 64;
  }
  if (vy > 0 && vy > -10 * 64) {
    vy -= 2 * 64;
  }

  opx = x + vx;
  opy = y + vy + (64 * 6);

  // Bullet & bomb setup
  Players.SetMaidShot();

  if (bomb_time != 0U) {
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
  deathbomb_count = 0;

  bomb_time = 0;
  deathbomb_time = 0;
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
  deathbomb_time = 0;
  ShiftCounter = 0;

  BuzzSound = false;
}

void Player::OnHit() {
#ifdef PBG_DEBUG
  if (!DebugDat.Hit)
    return;
#endif
  if (muteki != 0)
    return;
  if (deathbomb_time != 0)
    return;

  // Practice modes are handled inside OnDeath
  if (ConfigDat.PracticeMode.v >= PRACTICE_AUTOBOMB) {
    OnDeath();
    return;
  }

  // Play death sound immediately for feedback
  Snd_SEPlay(SOUND_ID_DEAD);

  if (bomb > 0 && bomb_time == 0) {
    const auto window = DEATHBOMB_WINDOW + (GAME_LUNATIC - static_cast<int>(GameState.game_level)) * 2;
    deathbomb_time = static_cast<uint16_t>(window);
    muteki = static_cast<uint16_t>(window);
    Effects.SpawnFragment(x, y, FRG_FATCIRCLE);
    return;
  }

  // No bomb stock — instant death, sound already played
  OnDeath(false);
}

void Player::OnDeath(bool play_se) {
  int i = 0;

#ifdef PBG_DEBUG
  if (!DebugDat.Hit)
    return;
#endif

  if (ConfigDat.PracticeMode.v == PRACTICE_INVINCIBLE) {
    Effects.SpawnFragment(x, y, FRG_FATCIRCLE);
    for (i = 0; i < 50; i++) {
      Effects.SpawnFragment(x, y, FRG_HEART);
    }
    if (play_se)
      Snd_SEPlay(SOUND_ID_DEAD);
    muteki = 30;
    return;
  }

  // Auto bomb: in practice mode with auto-bomb or higher, if bomb key is not
  // pressed and bomb stock remains, automatically activate bomb instead of dying
  if (ConfigDat.PracticeMode.v == PRACTICE_AUTOBOMB &&
      ((Key_Data & KEY_BOMB) == 0) && (bomb_time == 0) && (bomb > 0) &&
      (!Scroller.scene.MsgFlag)) {
    static constexpr uint8_t bomb_time_tbl[4] = {60 * 4, 60 * 3, 60 * 2, 0};
    bomb_time = bomb_time_tbl[weapon & 3];
    muteki = BOMBMUTEKI_VAL;
    bomb--;
    bomb_used++;
    Ranking.Add(-25); // auto bomb decreases rank
    Bullets.Clear();
    Lasers.Clear();
    return;
  }

  Effects.SpawnFragment(x, y, FRG_FATCIRCLE);

  for (i = 0; i < 50; i++) {
    Effects.SpawnFragment(x, y, FRG_HEART);
  }

  if (play_se)
    Snd_SEPlay(SOUND_ID_DEAD);

  x = opx = SX_START;
  y = opx = SY_START;
  vx = 0;
  vy = 0;

  lay_time = 0;
  lay_grp = 0;

  bomb = ConfigDat.BombStock.v;
  muteki = VIVDEAD_VAL;

  Ranking.Add(-100); // death decreases rank

  if (left != 0U) {
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

void Player::AddEvade(uint8_t n) { AddEvadeEx(x, y, n); }

void Player::AddEvadeEx(int ex, int ey, uint8_t n) {
  int i = 0;

  if (n != 0U) {
    if (!BuzzSound) {
      Snd_SEPlay(SOUND_ID_BUZZ, ex);
      BuzzSound = true;
    }
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
  }

  for (i = 0; std::cmp_less(i, n); i++) {
    if (evade == 999) {
      evade_c = 1;
      return;
    }
    evade += 1;
    evade_sum += 1;
    evadesc += evade * 20;
  }

  if (evade != 0U) {
    evade_c = EVADETIME_MAX;
  }
}

void Player::AddScore(int sc) { dscore += sc; }

void Player::PowerUp(uint8_t damage) {
  exp2 += damage;

  switch ((Cast::up<uint16_t>(exp) + 1) >> 5) {
  case 0:
    if (exp2 > 5 - 3) {
      exp++, exp2 = 0;
    }
    return;
  case 1:
    if (exp2 > 25 - 15) {
      exp++, exp2 = 0;
    }
    return;
  case 2:
    if (exp2 > 50 - 20) {
      exp++, exp2 = 0;
    }
    return;
  case 3:
    if (exp2 > 80) {
      exp++, exp2 = 0;
    }
    return;
  case 4:
    if (exp2 > 120) {
      exp++, exp2 = 0;
    }
    return;
  case 5:
    if (exp2 > 140) {
      exp++, exp2 = 0;
    }
    return;
  case 6:
    if (exp2 > 160) {
      exp++, exp2 = 0;
    }
    return;
  case 7:
    if (exp2 > 180) {
      exp++, exp2 = 0;
    }
    return;
  case 8:
    return; // at full power-up
  }
}

uint8_t Player::GetLaserDeg() const { return ((120 - bomb_time) * 3) / 2; }

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

// Backward compatibility: free function wrappers referenced from MAIDTAMA.cpp etc.
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i) {
  return Player::GetRightLaserDeg(LaserDeg, i);
}
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i) {
  return Player::GetLeftLaserDeg(LaserDeg, i);
}
