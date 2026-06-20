///
/// Player - Player-related processing
///

#include <algorithm>

#include <format>
#include <utility>

#include "config.h"
#include "font_uty.h"
#include "game/input.h"
#include "game/snd.h"
#include "geometry.h"
#include "gian.h"
#include "player.h"
#include "level.h"
#include "weapon/homing_form.h"
#include "weapon/laser_form.h"
#include "weapon/wide_form.h"

// --- Player method implementations ---

// Constructor: create weapon_ form strategy objects.
Player::Player() {
  forms_[0] = std::make_unique<WideForm>(*this);
  forms_[1] = std::make_unique<WideFocusForm>(*this);
  forms_[2] = std::make_unique<HomingForm>(*this);
  forms_[3] = std::make_unique<HomingFocusForm>(*this);
  forms_[4] = std::make_unique<LaserForm>(*this);
  forms_[5] = std::make_unique<LaserFocusForm>(*this);
}

Player::~Player() = default;

// Copy operations: duplicate player state but keep our own weapon_ forms
// (forms hold a reference to *this and must not be shared).
Player::Player(const Player &other) : Player() { *this = other; }

Player &Player::operator=(const Player &other) {
  if (this == &other) {
    return *this;
  }
  x_ = other.x_;
  y_ = other.y_;
  vx_ = other.vx_;
  vy_ = other.vy_;
  opx_ = other.opx_;
  opy_ = other.opy_;
  score_ = other.score_;
  dscore_ = other.dscore_;
  evade_sum_ = other.evade_sum_;
  evadesc_ = other.evadesc_;
  evade_ = other.evade_;
  evade_c_ = other.evade_c_;
  star_counter_ = other.star_counter_;
  star_threshold_ = other.star_threshold_;
  star_extend_count_ = other.star_extend_count_;
  v_ = other.v_;
  weapon_ = other.weapon_;
  exp_ = other.exp_;
  bomb_ = other.bomb_;
  left_ = other.left_;
  credit_ = other.credit_;
  miss_count_ = other.miss_count_;
  bomb_used_ = other.bomb_used_;
  deathbomb_count_ = other.deathbomb_count_;
  grp_id_ = other.grp_id_;
  bomb_time_ = other.bomb_time_;
  exp2_ = other.exp2_;
  muteki_ = other.muteki_;
  deathbomb_time_ = other.deathbomb_time_;
  lay_time_ = other.lay_time_;
  lay_grp_ = other.lay_grp_;
  toge_time_ = other.toge_time_;
  toge_ex_ = other.toge_ex_;
  shift_counter_ = other.shift_counter_;
  game_over_ = other.game_over_;
  buzz_sound_ = other.buzz_sound_;
  // maid_tama_ / maid_tama_ind_ / maid_tama_now_ are not copied: the shot
  // pool belongs to the live Player instance, not a state snapshot.
  // forms_ is intentionally preserved (not copied).
  return *this;
}

WeaponForm *Player::BaseForm_() const { return forms_[weapon_ * 2].get(); }

WeaponForm *Player::ActiveForm_() const {
  const bool focus = (Key_Data & KEY_SHIFT) != 0;
  return forms_[weapon_ * 2 + (focus ? 1 : 0)].get();
}

bool Player::IsMainShotFrame_(uint16_t t) const {
  return (t == MAID_MAIN_SHOT || t == MAID_MAIN_SHOT * 2 ||
          t == MAID_MAIN_SHOT * 3);
}

bool Player::IsSubShotFrame_(uint16_t t) const {
  return (t == 0 || t == MAID_SUB_SHOT) && bomb_time_ == 0;
}

void Player::SpawnShot_() {
  for (decltype(Bullets.command.n) i = 0; i < Bullets.command.n; i++) {
    if (maid_tama_now_ + 1 >= MAIDTAMA_MAX) {
      return;
    }

    auto *t = &maid_tama_[maid_tama_ind_[maid_tama_now_++]];

    t->x = t->tx = Bullets.command.x;
    t->y = t->ty = Bullets.command.y;

    t->v = t->v0 = Bullets.Speed(i);
    t->a = Bullets.command.a;

    t->d = Bullets.Dir(i);
    t->d16 = (t->d << 8);

    t->vx = cosl(t->d, t->v);
    t->vy = sinl(t->d, t->v);

    t->vd = Bullets.command.vd;
    t->c = Bullets.command.c;
    t->rep = Bullets.command.rep;
    t->type = Bullets.command.type;
    t->option = Bullets.command.option;
    t->effect = 0;
    t->count = 0;
    t->flag = Bullets.Flag();
  }
}

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

  if (weapon_ != 0 || bomb_time_ == 0) {
    return;
  }

  x = BX_MIN;
  y = BY_MIN;

  if (bomb_time_ > 80) {
    t = (((60 * 4) - bomb_time_) / 4);
    t = std::max(t, 0);
    t = std::min(t, 5);
  } else {
    t = (bomb_time_ / 4);
    t = std::min(t, 5);
  }

  GrpSurface_Blit({x, y}, SURFACE_ID::BOMBER, data[t]);
}

void Player::DrawLaserBomb_() const {
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
        p[0].x = (opx_ >> 6) + SBOPT_DX + 0 + wx;
        p[0].y = (opy_ >> 6) + 0 + wy;
        p[3].x = (opx_ >> 6) + SBOPT_DX + 0 - wx;
        p[3].y = (opy_ >> 6) + 0 - wy;
        p[2].x = (opx_ >> 6) + SBOPT_DX + lx - wx;
        p[2].y = (opy_ >> 6) + ly - wy;
        p[1].x = (opx_ >> 6) + SBOPT_DX + lx + wx;
        p[1].y = (opy_ >> 6) + ly + wy;
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
        p[0].x = (opx_ >> 6) - SBOPT_DX + 0 + wx;
        p[0].y = (opy_ >> 6) + 0 + wy;
        p[3].x = (opx_ >> 6) - SBOPT_DX + 0 - wx;
        p[3].y = (opy_ >> 6) + 0 - wy;
        p[2].x = (opx_ >> 6) - SBOPT_DX + lx - wx;
        p[2].y = (opy_ >> 6) + ly - wy;
        p[1].x = (opx_ >> 6) - SBOPT_DX + lx + wx;
        p[1].y = (opy_ >> 6) + ly + wy;
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
        p[0].x = (opx_ >> 6) + SBOPT_DX + 0 + wx;
        p[0].y = (opy_ >> 6) + 0 + wy;
        p[3].x = (opx_ >> 6) + SBOPT_DX + 0 - wx;
        p[3].y = (opy_ >> 6) + 0 - wy;
        p[2].x = (opx_ >> 6) + SBOPT_DX + lx - wx;
        p[2].y = (opy_ >> 6) + ly - wy;
        p[1].x = (opx_ >> 6) + SBOPT_DX + lx + wx;
        p[1].y = (opy_ >> 6) + ly + wy;
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
        p[0].x = (opx_ >> 6) - SBOPT_DX + 0 + wx;
        p[0].y = (opy_ >> 6) + 0 + wy;
        p[3].x = (opx_ >> 6) - SBOPT_DX + 0 - wx;
        p[3].y = (opy_ >> 6) + 0 - wy;
        p[2].x = (opx_ >> 6) - SBOPT_DX + lx - wx;
        p[2].y = (opy_ >> 6) + ly - wy;
        p[1].x = (opx_ >> 6) - SBOPT_DX + lx + wx;
        p[1].y = (opy_ >> 6) + ly + wy;
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
      {{480, 128, 480 + 24, 128 + 24}, {504, 128, 504 + 24, 128 + 24}}, // wide
      {{480, 152, 480 + 24, 152 + 24},
       {504, 152, 504 + 24, 152 + 24}}, // homing
      {{528, 152, 528 + 24, 152 + 24}, {552, 152, 552 + 24, 152 + 24}}, // laser
      {{480, 152, 480 + 24, 152 + 24}, {504, 152, 504 + 24, 152 + 24}}, // temp
  };

  static uint8_t draw_flag = 0;
  static uint8_t draw_flag2 = 0;

  const auto sx = ((x_ >> 6) - 16);
  const auto sy = ((y_ >> 6) - 24);
  const auto ox = ((opx_ >> 6) - 12);
  const auto oy = ((opy_ >> 6) - 12);
  PIXEL_LTRB src;

  draw_flag = 1 - draw_flag;
  draw_flag2++;

  if (muteki_ == VIVDEAD_VAL) {
    draw_flag = 0;
  }

  if (muteki_ == 0 || (draw_flag != 0U)) {
    src = PIXEL_LTWH{(384 + (grp_id_ * 32)), 128, (16 * 2), (16 * 3)};
    GrpSurface_Blit({sx, sy}, SURFACE_ID::SYSTEM, src);
  }

  if ((Key_Data & KEY_SHIFT) != 0 && muteki_ < VIVDEAD_VAL) {
    const auto cx = (x_ >> 6);
    const auto cy = (y_ >> 6);

    GrpGeom->Lock();

    GrpGeom->SetColor({5, 5, 5});
    GeomCircleF({cx, cy}, 3);

    GrpGeom->SetColor({5, 2, 2});
    GeomCircleF({cx, cy}, 1);

    GrpGeom->Unlock();
  }

  if (((exp_ + 1) >> 5) != 0) {
    if (muteki_ < VIVDEAD_VAL) {
      const int opt_off = (weapon_ == 2 && (Key_Data & KEY_SHIFT) != 0)
                              ? (SBOPT_DX / 2)
                              : SBOPT_DX;
      src = VivBit[weapon_ & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox + opt_off), oy}, SURFACE_ID::SYSTEM, src);
      src = VivBit[weapon_ & 3][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox - opt_off), oy}, SURFACE_ID::SYSTEM, src);
    }
  }

  if ((bomb_time_ != 0U) && weapon_ == 2) {
    DrawLaserBomb_();
  }
}

void Player::DrawStatus() const {
  int i = 0;
  int temp = 0;
  constexpr PIXEL_LTRB src = {0, 80, 128, (80 + 24)};

  if (evade_c_ != 0U) {
    GrpGeom->Lock();
    GrpGeom->SetColor({5, 1, 0});
    GrpGeom->SetAlphaOne();
    for (i = 0; i <= 10; i++) {
      temp = 128 + 9 + (evade_c_ >> 2) + (5 - i);
      if (temp > (128 + 8)) {
        GrpGeom->DrawBoxA((128 + 8), (16 + 3 + i), temp, (16 + 3 + i + 1));
      }
    }
    GrpGeom->Unlock();
  }
  GrpSurface_Blit({128, 16}, SURFACE_ID::SYSTEM, src);
  GrpPut57(128 + 95, 16 + 91 - 80, std::format("{:3}", evade_).c_str());

  GrpPut16(128, 0, std::format("{:9}", score_).c_str());

  GrpPut16(280, 0, std::format("       Bomb {}", bomb_).c_str());

  for (i = 0; std::cmp_less(i, left_); i++) {
    constexpr PIXEL_LTWH life_src = {608, 432, 16, 16};
    GrpSurface_Blit({(280 + (i * 14)), 0}, SURFACE_ID::SYSTEM, life_src);
  }
}

void Player::Update() {
  int vx = 0;
  int vy = 0;
  int v = 0;
  constexpr int speed_tbl[] = {VIV_SPEED_WIDE, VIV_SPEED_HOMING,
                               VIV_SPEED_LASER};

  // Decrease graze remaining time
  if (evade_c_ != 0U) {
    if ((bomb_time_ != 0U) && evade_c_ >= 2) {
      evade_c_ -= 2;
    } else {
      evade_c_ -= 1;
    }

    if (evade_c_ == 0) {
      if (evade_ > 100) {
        Effects.SpawnStringEffect(
            180, 40,
            std::format("{:3} Evade  {:7} Pts", evade_, evadesc_).c_str());
      }

      AddScore(evadesc_);
      evade_ = 0;
      evadesc_ = 0;
    }
  }

  // Decrease invincibility time (not during bomb_)
  if ((muteki_ != 0U) && bomb_time_ == 0) {
    muteki_--;
  }

  // Deathbomb window countdown
  if (deathbomb_time_ != 0U) {
    deathbomb_time_--;
    if (deathbomb_time_ == 0U) {
      OnDeath(false); // sound already played in OnHit
    }
  }

  // Score change processing
  if (dscore_ >= 100000) {
    score_ += 100000, dscore_ -= 100000;
  } else if (dscore_ >= 20000) {
    score_ += 20000, dscore_ -= 20000;
  } else if (dscore_ >= 2000) {
    score_ += 2000, dscore_ -= 2000;
  } else if (dscore_ >= 200) {
    score_ += 200, dscore_ -= 200;
  } else if (dscore_ >= 20) {
    score_ += 20, dscore_ -= 20;
  } else if (dscore_ >= 10) {
    score_ += 10, dscore_ -= 10;
  }

  // Enable held-button slowdown
  if (ConfigDat.z_spd_down_enabled) {
    if ((Key_Data & KEY_TAMA) != 0) {
      if (shift_counter_ < 8) {
        shift_counter_++;
      } else {
        Key_Data = Key_Data | KEY_SHIFT;
      }
    } else {
      shift_counter_ = 0;
    }
  }

  if (muteki_ < MAID_MOVE_DISABLE_TIME) {
    vx = vy = 0;
    v = ((Key_Data & KEY_SHIFT) != 0) ? (speed_tbl[weapon_] / 3)
                                      : speed_tbl[weapon_];
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
      x_ += (vx / 6);
      y_ += (vy / 6);
    } else {
      x_ += (vx >> 2);
      y_ += (vy >> 2);
    }

    if (y_ < SY_MIN) {
      y_ = SY_MIN;
    } else if (y_ > SY_MAX) {
      y_ = SY_MAX;
    }

    if (x_ < SX_MIN) {
      x_ = SX_MIN;
    } else if (x_ > SX_MAX) {
      x_ = SX_MAX;
    }
  } else {
    vx = 0;
    vy = -(64 + 32);
    y_ += vy;
  }

  if (vx > 0) {
    grp_id_ = 2;
  } else if (vx < 0) {
    grp_id_ = 0;
  } else {
    grp_id_ = 1;
  }

  opx_ = x_;
  opy_ = y_;

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

  opx_ = x_ + vx;
  opy_ = y_ + vy + (64 * 6);

  // Bullet & bomb_ setup
  SetMaidShot();

  if (bomb_time_ != 0U) {
    Bullets.Clear();
    Lasers.Clear();
  }

  buzz_sound_ = false;
}

void Player::Initialize() {
  PrepareNextStage();

  score_ = 0;
  dscore_ = 0;
  exp_ = 0;
  exp2_ = 0;
  bomb_ = ConfigDat.bomb_stock;
  left_ = ConfigDat.player_stock;
  credit_ = 4;

  miss_count_ = 0;
  bomb_used_ = 0;
  deathbomb_count_ = 0;

  star_counter_ = 0;
  star_threshold_ = STAR_THRESHOLD_INIT;
  star_extend_count_ = 0;

  bomb_time_ = 0;
  deathbomb_time_ = 0;
  evade_c_ = evade_ = 0;
  evadesc_ = 0;
  evade_sum_ = 0;

  grp_id_ = 1;

  muteki_ = VIVDEAD_VAL;

  game_over_ = false;

  toge_ex_ = 0;
  toge_time_ = 0;
  lay_time_ = 0;
  lay_grp_ = 0;
  shift_counter_ = 0;

  buzz_sound_ = false;
}

void Player::PrepareNextStage() {
  x_ = opx_ = SX_START;
  y_ = opx_ = SY_START;
  vx_ = 0;
  vy_ = 0;

  toge_ex_ = 0;
  toge_time_ = 0;
  lay_time_ = 0;
  lay_grp_ = 0;

  muteki_ = VIVDEAD_VAL;
  bomb_time_ = 0;
  deathbomb_time_ = 0;
  shift_counter_ = 0;

  buzz_sound_ = false;
}

void Player::OnHit() {
#ifdef PBG_DEBUG
  if (!DebugDat.Hit)
    return;
#endif
  if (muteki_ != 0)
    return;
  if (deathbomb_time_ != 0)
    return;

  // Practice modes are handled inside OnDeath
  if (ConfigDat.practice_mode >= PracticeMode::AUTOBOMB) {
    OnDeath(true);
    return;
  }

  if (bomb_ > 0 && bomb_time_ == 0) {
    // Enter deathbomb window with immediate feedback
    Snd_SEPlay(SOUND_ID_DEAD);
    Effects.SpawnFragment(x_, y_, FRG_FATCIRCLE);
    const auto window =
        DEATHBOMB_WINDOW +
        (static_cast<int>(GameLevel::LUNATIC) -
         static_cast<int>(std::to_underlying(GameState.game_level))) * 2;
    deathbomb_time_ = static_cast<uint16_t>(window);
    muteki_ = static_cast<uint16_t>(window);
    return;
  }

  // No bomb_ stock — instant death
  OnDeath(true);
}

void Player::OnDeath(bool play_se) {
  int i = 0;

#ifdef PBG_DEBUG
  if (!DebugDat.Hit)
    return;
#endif

  if (ConfigDat.practice_mode == PracticeMode::INVINCIBLE) {
    Effects.SpawnFragment(x_, y_, FRG_FATCIRCLE);
    for (i = 0; i < 50; i++) {
      Effects.SpawnFragment(x_, y_, FRG_HEART);
    }
    if (play_se)
      Snd_SEPlay(SOUND_ID_DEAD);
    muteki_ = 30;
    return;
  }

  if (play_se) {
    Snd_SEPlay(SOUND_ID_DEAD);
    Effects.SpawnFragment(x_, y_, FRG_FATCIRCLE);
  }

  // Auto bomb_: in practice mode with auto-bomb_ or higher, if bomb_ key is not
  // pressed and bomb_ stock remains, automatically activate bomb_ instead of
  // dying
  if (ConfigDat.practice_mode == PracticeMode::AUTOBOMB &&
      ((Key_Data & KEY_BOMB) == 0) && (bomb_time_ == 0) && (bomb_ > 0) &&
      (!Scroller.scene.MsgFlag)) {
    static constexpr uint8_t bomb_time_tbl[4] = {60 * 4, 60 * 3, 60 * 2, 0};
    bomb_time_ = bomb_time_tbl[weapon_ & 3];
    muteki_ = BOMBMUTEKI_VAL;
    bomb_--;
    bomb_used_++;
    Ranking.Add(-BOMB_RANK_DECR); // auto bomb_ decreases rank
    Bullets.Clear();
    Lasers.Clear();
    return;
  }

  for (i = 0; i < 50; i++) {
    Effects.SpawnFragment(x_, y_, FRG_HEART);
  }

  x_ = opx_ = SX_START;
  y_ = opx_ = SY_START;
  vx_ = 0;
  vy_ = 0;

  lay_time_ = 0;
  lay_grp_ = 0;

  bomb_ = ConfigDat.bomb_stock;
  muteki_ = VIVDEAD_VAL;

  Ranking.Add(-DEATH_RANK_DECR); // death decreases rank

  if (left_ != 0U) {
    left_ -= 1;
    miss_count_++;
  } else {
    game_over_ = true;
    score_ += dscore_;

    evade_c_ = 0;
    evade_ = 0;
    evadesc_ = 0;

    GameOverInit();
  }

  Bullets.Clear();
  Lasers.Clear();
}

void Player::AddEvade(uint8_t n) { AddEvadeEx(x_, y_, n); }

void Player::AddEvadeEx(int ex, int ey, uint8_t n) {
  if (n != 0U) {
    if (!buzz_sound_) {
      Snd_SEPlay(SOUND_ID_BUZZ, ex);
      buzz_sound_ = true;
    }
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
    Effects.SpawnFragment(ex, ey, FRG_EVADE);
  }

  for (uint8_t i = 0; i < n; i++) {
    if (evade_ == 999) {
      evade_c_ = 1;
      return;
    }
    evade_ += 1;
    evade_sum_ += 1;
    evadesc_ += evade_ * 20;
  }

  if (evade_ != 0U) {
    evade_c_ = std::min(static_cast<uint16_t>(evade_c_ + EVADETIME_INCR),
                        EVADETIME_MAX);
  }
}

void Player::AddScore(int sc) { dscore_ += sc; }

void Player::PowerUp(uint8_t damage) {
  exp2_ += damage;

  switch ((Cast::up<uint16_t>(exp_) + 1) >> 5) {
  case 0:
    if (exp2_ > 5 - 3) {
      exp_++, exp2_ = 0;
    }
    return;
  case 1:
    if (exp2_ > 25 - 15) {
      exp_++, exp2_ = 0;
    }
    return;
  case 2:
    if (exp2_ > 50 - 20) {
      exp_++, exp2_ = 0;
    }
    return;
  case 3:
    if (exp2_ > 80) {
      exp_++, exp2_ = 0;
    }
    return;
  case 4:
    if (exp2_ > 120) {
      exp_++, exp2_ = 0;
    }
    return;
  case 5:
    if (exp2_ > 140) {
      exp_++, exp2_ = 0;
    }
    return;
  case 6:
    if (exp2_ > 160) {
      exp_++, exp2_ = 0;
    }
    return;
  case 7:
    if (exp2_ > 180) {
      exp_++, exp2_ = 0;
    }
    return;
  case 8:
    return; // at full power-up
  }
}

uint8_t Player::GetLaserDeg() const { return ((120 - bomb_time_) * 3) / 2; }

uint8_t Player::GetLeftOrRightLaserDeg_(uint8_t LaserDeg, int i) {
  return ((LaserDeg < 58)
              ? ((LaserDeg * 3) + ((i * (64 - LaserDeg)) / 2))
              : ((58 * 3) + ((i * (64 - (std::min)(62, int{LaserDeg}))) / 2)));
}

uint8_t Player::GetRightLaserDeg(uint8_t LaserDeg, int i) {
  return (64 + 48 - GetLeftOrRightLaserDeg_(LaserDeg, i));
}

uint8_t Player::GetLeftLaserDeg(uint8_t LaserDeg, int i) {
  return (64 - 48 + GetLeftOrRightLaserDeg_(LaserDeg, i));
}

// --- New action methods ---

void Player::RotateWeapon(int dir) {
  // dir: +1 = forward, -1 = backward; wraps among 3 weapons.
  weapon_ = (weapon_ + (dir < 0 ? 2 : 1)) % 3;
}

PlayerReward Player::AddStar(uint32_t n) {
  star_counter_ += n;
  if (star_counter_ >= star_threshold_) {
    star_threshold_ += STAR_THRESHOLD_INCR;
    star_extend_count_++;

    // reward loop: EB...B|EB...B
    if (star_extend_count_ % STAR_EXTEND_LOOP == 1) {
      left_++;
      return PlayerReward::EXTEND;
    }

    bomb_++;
    return PlayerReward::BOMB;
  }

  return PlayerReward::NONE;
}

void Player::ResetForContinue() {
  evade_sum_ = 0;
  left_ = ConfigDat.player_stock;
  score_ = ((score_ % 10) + 1);
  star_counter_ = 0;
  star_threshold_ = STAR_THRESHOLD_INIT;
  star_extend_count_ = 0;
}

void Player::ApplyReplayState(uint8_t weapon, uint8_t exp, uint8_t left,
                              uint8_t bombs) {
  weapon_ = weapon;
  exp_ = exp;
  left_ = left;
  bomb_ = bombs;
}

// --- Weapon-select preview ---

void Player::SaveSnapshot_(StateSnapshot &s) const {
  s.x = x_;
  s.y = y_;
  s.vx = vx_;
  s.vy = vy_;
  s.opx = opx_;
  s.opy = opy_;
  s.score = score_;
  s.dscore = dscore_;
  s.evade_sum = evade_sum_;
  s.evadesc = evadesc_;
  s.evade = evade_;
  s.evade_c = evade_c_;
  s.star_counter = star_counter_;
  s.star_threshold = star_threshold_;
  s.star_extend_count = star_extend_count_;
  s.v = v_;
  s.weapon = weapon_;
  s.exp = exp_;
  s.bomb = bomb_;
  s.left = left_;
  s.credit = credit_;
  s.miss_count = miss_count_;
  s.bomb_used = bomb_used_;
  s.deathbomb_count = deathbomb_count_;
  s.grp_id = grp_id_;
  s.bomb_time = bomb_time_;
  s.exp2 = exp2_;
  s.muteki = muteki_;
  s.deathbomb_time = deathbomb_time_;
  s.lay_time = lay_time_;
  s.lay_grp = lay_grp_;
  s.toge_time = toge_time_;
  s.toge_ex = toge_ex_;
  s.shift_counter = shift_counter_;
  s.game_over = game_over_;
  s.buzz_sound = buzz_sound_;
}

void Player::RestoreSnapshot_(const StateSnapshot &s) {
  x_ = s.x;
  y_ = s.y;
  vx_ = s.vx;
  vy_ = s.vy;
  opx_ = s.opx;
  opy_ = s.opy;
  score_ = s.score;
  dscore_ = s.dscore;
  evade_sum_ = s.evade_sum;
  evadesc_ = s.evadesc;
  evade_ = s.evade;
  evade_c_ = s.evade_c;
  star_counter_ = s.star_counter;
  star_threshold_ = s.star_threshold;
  star_extend_count_ = s.star_extend_count;
  v_ = s.v;
  weapon_ = s.weapon;
  exp_ = s.exp;
  bomb_ = s.bomb;
  left_ = s.left;
  credit_ = s.credit;
  miss_count_ = s.miss_count;
  bomb_used_ = s.bomb_used;
  deathbomb_count_ = s.deathbomb_count;
  grp_id_ = s.grp_id;
  bomb_time_ = s.bomb_time;
  exp2_ = s.exp2;
  muteki_ = s.muteki;
  deathbomb_time_ = s.deathbomb_time;
  lay_time_ = s.lay_time;
  lay_grp_ = s.lay_grp;
  toge_time_ = s.toge_time;
  toge_ex_ = s.toge_ex;
  shift_counter_ = s.shift_counter;
  game_over_ = s.game_over;
  buzz_sound_ = s.buzz_sound;
}

void Player::BeginWeaponPreview() {
  StateSnapshot s;
  SaveSnapshot_(s);
  preview_snapshot_ = s;
  weapon_ = 0;
}

void Player::CommitWeaponSelection() {
  if (!preview_snapshot_) {
    return;
  }
  const uint8_t selected = weapon_;
  RestoreSnapshot_(*preview_snapshot_);
  weapon_ = selected;
  preview_snapshot_.reset();
}
