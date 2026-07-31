///
/// Player - Player-related processing
///

#include <algorithm>
#include <format>
#include <utility>

#include "loadout/homing_loadout.h"
#include "loadout/laser_loadout.h"
#include "loadout/wide_loadout.h"
#include "player.h"

#include "audio/sfx.h"
#include "effect/effect_manager.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "stage/stage_session.h"
#include "sys/input.h"
#include "util/math_utils.h"

namespace {

inline constexpr auto kMovementLeft = playfield::kWorldLeft + 10_px;
inline constexpr auto kMovementRight = playfield::kWorldRight - 10_px;
inline constexpr auto kMovementTop = playfield::kWorldTop + 40_px;
inline constexpr auto kMovementBottom = playfield::kWorldBottom - 10_px;
inline constexpr auto kStartX = playfield::kWorldCenterX;
inline constexpr auto kStartY = playfield::kWorldBottom + 180_px;

} // namespace

// --- Player method implementations ---

Player::Player(EffectManager &effects, GameSession &session,
               stage::StageSession &stage)
    : loadout_(std::make_unique<WideLoadout>()), effects_(effects),
      session_(session), stage_(stage) {}

bool Player::IsMainShotFrame(uint16_t t) const {
  return (t == kMainShotFrame || t == kMainShotFrame * 2 ||
          t == kMainShotFrame * 3);
}

bool Player::IsSubShotFrame(uint16_t t) const {
  return (t == 0 || t == kSubShotFrame) && bomb_time_ == 0;
}

void Player::SpawnShot(const PlayerShotSpawnInfo &si) {
  for (uint8_t i = 0; i < si.count; i++) {
    auto *t = maid_tama_.Alloc();
    if (!t) {
      return;
    }

    uint8_t di = i % si.count;
    di++;
    uint8_t d;
    if ((si.count & 1) != 0) {
      d = si.direction +
          ((di >> 1) * si.direction_step * (1 - ((di & 1) << 1)));
    } else {
      d = si.direction - (si.direction_step >> 1) +
          ((di >> 1) * si.direction_step * (1 - ((di & 1) << 1)));
    }

    t->x_ = si.x;
    t->y_ = si.y;
    t->speed_ = si.speed;
    t->acceleration_ = si.acceleration;
    t->direction_ = math::AngleFromLegacy(d);
    const auto velocity =
        math::RoundedPolarVector(math::AngleFromLegacy(d), si.speed);
    t->velocity_x_ = velocity.x;
    t->velocity_y_ = velocity.y;
    t->turn_rate_ = si.turn_rate;
    t->kind_ = si.kind;
    t->motion_ = si.motion;
    t->age_ = 0;
    t->pending_removal_ = false;
  }
}

void Player::DrawBombBackground() const {
  loadout_->DrawBombBackground(*this, bomb_time_);
}

int Player::HitRadiusPixels() const {
  return (HitRadius() + WORLD_COORD_SCALE - 1) / WORLD_COORD_SCALE;
}

void Player::DrawFocusHitbox() const {
  if (!focus_hitbox_visible_ || !focused_ ||
      invincibility_time_ >= kRespawnInvincibilityDuration) {
    return;
  }

  const WINDOW_POINT center{x_ >> WORLD_COORD_BITS, y_ >> WORLD_COORD_BITS};

  GrpGeom->SetColor({5, 5, 5});
  GeomCircleF(center, HitRadiusPixels());
  GrpGeom->SetColor({5, 2, 2});
  GeomCircleF(center, 1);
}

void Player::DrawDebugHitbox() const {
  const WINDOW_POINT center{x_ >> WORLD_COORD_BITS, y_ >> WORLD_COORD_BITS};
  GrpGeom->SetColor({0, 0, 0});
  GrpGeom->SetAlphaNorm(204);
  geometry::DrawFilledCircle(*GrpGeom, center, HitRadiusPixels(), true);
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

  if (invincibility_time_ == kRespawnInvincibilityDuration) {
    draw_flag = 0;
  }

  if (!IsInvincible() || (draw_flag != 0U)) {
    src = PIXEL_LTWH{(384 + (grp_id_ * 32)), 128, (16 * 2), (16 * 3)};
    GrpSurface_Blit({sx, sy}, SURFACE_ID::SYSTEM, src);
  }

  DrawFocusHitbox();

  if (((exp_ + 1) >> 5) != 0) {
    if (invincibility_time_ < kRespawnInvincibilityDuration) {
      const int opt_off = loadout_->OptionOffset(focused_);
      src = VivBit[loadout_->OptionSprite()][(draw_flag2 >> 2) & 1];
      GrpSurface_Blit({(ox + opt_off), oy}, SURFACE_ID::SYSTEM, src);
      GrpSurface_Blit({(ox - opt_off), oy}, SURFACE_ID::SYSTEM, src);
    }
  }

  loadout_->DrawBombForeground(*this, bomb_time_);
}

void Player::UpdateStatus() {
  // Decrease graze remaining time
  if (evade_c_ != 0U) {
    if ((bomb_time_ != 0U) && evade_c_ >= 2) {
      evade_c_ -= 2;
    } else {
      evade_c_ -= 1;
    }

    if (evade_c_ == 0) {
      if (evade_ > 100) {
        effects_.SpawnString(
            180, 40,
            std::format("{:3} Evade  {:7} Pts", evade_, evadesc_).c_str());
      }

      AddScore(evadesc_);
      evade_ = 0;
      evadesc_ = 0;
    }
  }

  // Decrease invincibility time (not during bomb_)
  if ((invincibility_time_ != 0U) && bomb_time_ == 0) {
    invincibility_time_--;
  }
  if (life_state_ == LifeState::Respawning &&
      invincibility_time_ < kRespawnMovementThreshold) {
    life_state_ = LifeState::Active;
  }

  // Deathbomb window countdown
  if (deathbomb_time_ != 0U) {
    deathbomb_time_--;
    if (deathbomb_time_ == 0U) {
      CommitDeath();
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
}

INPUT_BITS Player::PrepareInput(INPUT_BITS input) {
  if (std::exchange(auto_bomb_requested_, false)) {
    input |= KEY_BOMB;
  }

  if (focus_while_firing_) {
    if ((input & KEY_TAMA) != 0) {
      if (shift_counter_ < 8) {
        shift_counter_++;
      } else {
        input |= KEY_SHIFT;
      }
    } else {
      shift_counter_ = 0;
    }
  }

  focused_ = (input & KEY_SHIFT) != 0;
  return input;
}

void Player::UpdateMovement(INPUT_BITS input) {
  int vx = 0;
  int vy = 0;
  int v = 0;

  if (!IsMovementDisabled()) {
    vx = vy = 0;
    v = loadout_->MoveSpeed(focused_);
    if ((input & KEY_UP) != 0) {
      vy -= v;
    }
    if ((input & KEY_DOWN) != 0) {
      vy += v;
    }
    if ((input & KEY_LEFT) != 0) {
      vx -= v;
    }
    if ((input & KEY_RIGHT) != 0) {
      vx += v;
    }

    if ((vx != 0) && (vy != 0)) {
      x_ += (vx / 6);
      y_ += (vy / 6);
    } else {
      x_ += (vx >> 2);
      y_ += (vy >> 2);
    }

    if (y_ < kMovementTop) {
      y_ = kMovementTop;
    } else if (y_ > kMovementBottom) {
      y_ = kMovementBottom;
    }

    if (x_ < kMovementLeft) {
      x_ = kMovementLeft;
    } else if (x_ > kMovementRight) {
      x_ = kMovementRight;
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

  UpdateOptionPosition(vx, vy);
}

void Player::UpdateOptionPosition(int movement_x, int movement_y) {
  option_lag_x_ -= std::clamp(option_lag_x_, -1_px, 1_px);
  option_lag_y_ -= std::clamp(option_lag_y_, -1_px, 1_px);

  if (movement_x < 0 && option_lag_x_ < 6_px) {
    option_lag_x_ += 2_px;
  }
  if (movement_x > 0 && option_lag_x_ > -6_px) {
    option_lag_x_ -= 2_px;
  }
  if (movement_y < 0 && option_lag_y_ < 10_px) {
    option_lag_y_ += 2_px;
  }
  if (movement_y > 0 && option_lag_y_ > -10_px) {
    option_lag_y_ -= 2_px;
  }

  opx_ = x_ + option_lag_x_;
  opy_ = y_ + option_lag_y_ + 6_px;
}

PlayerUpdateResult Player::Update(EnemyManager &enemies, INPUT_BITS input) {
  UpdateStatus();
  input = PrepareInput(input);
  UpdateMovement(input);
  UpdateWeapons(enemies, input);

  if (bomb_time_ != 0U) {
    clear_bullets_requested_ = true;
  }

  buzz_sound_ = false;
  UpdateProjectiles(enemies);

  return {
      .effective_input = input,
      .clear_bullets = std::exchange(clear_bullets_requested_, false),
      .game_over = game_over_,
  };
}

void Player::Initialize(int player_stock, int bomb_stock) {
  PrepareNextStage();

  score_ = 0;
  dscore_ = 0;
  exp_ = 0;
  exp2_ = 0;
  initial_bomb_stock_ = static_cast<uint8_t>(bomb_stock);
  bomb_ = initial_bomb_stock_;
  left_ = player_stock;
  credit_ = 4;

  miss_count_ = 0;
  bomb_used_ = 0;
  deathbomb_count_ = 0;

  star_counter_ = 0;
  star_threshold_ = kInitialStarThreshold;
  star_extend_count_ = 0;

  bomb_time_ = 0;
  deathbomb_time_ = 0;
  evade_c_ = evade_ = 0;
  evadesc_ = 0;
  evade_sum_ = 0;

  grp_id_ = 1;

  invincibility_time_ = kRespawnInvincibilityDuration;
  life_state_ = LifeState::Respawning;

  game_over_ = false;
  toge_time_ = 0;
  shift_counter_ = 0;

  buzz_sound_ = false;
  auto_bomb_requested_ = false;
}

void Player::PrepareNextStage() {
  x_ = opx_ = kStartX;
  y_ = opy_ = kStartY;
  option_lag_x_ = option_lag_y_ = 0;

  toge_time_ = 0;
  loadout_->Reset();
  maid_tama_.Reset();

  invincibility_time_ = kRespawnInvincibilityDuration;
  bomb_time_ = 0;
  deathbomb_time_ = 0;
  life_state_ = LifeState::Respawning;
  shift_counter_ = 0;
  focused_ = false;
  buzz_sound_ = false;
  auto_bomb_requested_ = false;
}

void Player::OnHit() {
  if (IsInvincible()) {
    return;
  }

  switch (practice_mode_) {
  case PracticeMode::Off:
  case PracticeMode::AutoBomb:
    if (bomb_ != 0U && bomb_time_ == 0U) {
      EnterDeathbombWindow();
      if (practice_mode_ == PracticeMode::AutoBomb) {
        auto_bomb_requested_ = true;
      }
    } else {
      PlayHitFeedback();
      CommitDeath();
    }
    return;
  case PracticeMode::Invincible:
    PlayHitFeedback();
    for (int i = 0; i < 50; i++) {
      effects_.SpawnFragment(x_, y_, FragmentKind::Heart);
    }
    invincibility_time_ = kPracticeHitInvincibilityDuration;
    return;
  }
}

void Player::PlayHitFeedback() const {
  PlaySfx(SfxId::Dead);
  effects_.SpawnFragment(x_, y_, FragmentKind::ExpandingCircle);
}

void Player::EnterDeathbombWindow() {
  PlayHitFeedback();
  const auto window = kDeathbombWindow +
                      (static_cast<int>(GameLevel::Lunatic) -
                       static_cast<int>(std::to_underlying(session_.level))) *
                          2;
  deathbomb_time_ = static_cast<uint16_t>(window);
  life_state_ = LifeState::DeathbombWindow;
}

bool Player::ActivateBomb(BombTrigger trigger) {
  if (bomb_time_ != 0U || bomb_ == 0U || stage_.DialogueActive()) {
    return false;
  }

  const bool rescuing_deathbomb = life_state_ == LifeState::DeathbombWindow;
  if (trigger == BombTrigger::Manual &&
      (invincibility_time_ != 0U || rescuing_deathbomb)) {
    return false;
  }
  if (trigger == BombTrigger::Deathbomb && !rescuing_deathbomb) {
    return false;
  }

  bomb_time_ = loadout_->BombDuration();
  invincibility_time_ = kBombEndInvincibilityDuration;
  const uint8_t bomb_cost =
      trigger == BombTrigger::Deathbomb && bomb_ >= kDeathbombCost
          ? kDeathbombCost
          : 1;
  bomb_ -= bomb_cost;
  bomb_used_++;
  if (trigger == BombTrigger::Deathbomb) {
    deathbomb_count_++;
  }
  deathbomb_time_ = 0;
  life_state_ = LifeState::Active;
  session_.AddRank(-kBombRankDecrease);
  clear_bullets_requested_ = true;
  return true;
}

void Player::CommitDeath() {
  for (int i = 0; i < 50; i++) {
    effects_.SpawnFragment(x_, y_, FragmentKind::Heart);
  }

  x_ = opx_ = kStartX;
  y_ = opy_ = kStartY;
  option_lag_x_ = option_lag_y_ = 0;

  loadout_->Reset();

  bomb_ = initial_bomb_stock_;
  bomb_time_ = 0;
  deathbomb_time_ = 0;
  invincibility_time_ = kRespawnInvincibilityDuration;
  life_state_ = LifeState::Respawning;

  session_.AddRank(-kDeathRankDecrease); // death decreases rank

  if (left_ != 0U) {
    left_ -= 1;
    miss_count_++;
  } else {
    game_over_ = true;
    score_ += dscore_;

    evade_c_ = 0;
    evade_ = 0;
    evadesc_ = 0;
  }

  clear_bullets_requested_ = true;
}

void Player::AddEvade(uint8_t n) { AddEvadeEx(x_, y_, n); }

void Player::AddEvadeEx(int ex, int ey, uint8_t n) {
  if (n != 0U) {
    if (!buzz_sound_) {
      PlaySfx(SfxId::Buzz, ex);
      buzz_sound_ = true;
    }
    effects_.SpawnFragment(ex, ey, FragmentKind::Graze);
    effects_.SpawnFragment(ex, ey, FragmentKind::Graze);
    effects_.SpawnFragment(ex, ey, FragmentKind::Graze);
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
    evade_c_ = std::min(static_cast<uint16_t>(evade_c_ + kGrazeWaitIncrement),
                        kGrazeWaitMax);
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
  default:
    return; // at full power-up
  }
}

void Player::SelectType(PlayerType type) {
  if (loadout_ && loadout_->Type() == type) {
    return;
  }

  switch (type) {
  case PlayerType::Wide:
    loadout_ = std::make_unique<WideLoadout>();
    break;
  case PlayerType::Homing:
    loadout_ = std::make_unique<HomingLoadout>();
    break;
  case PlayerType::Laser:
    loadout_ = std::make_unique<LaserLoadout>();
    break;
  }
}

void Player::RotateType(int dir) {
  const auto current = std::to_underlying(Type());
  SelectType(static_cast<PlayerType>((current + (dir < 0 ? 2 : 1)) % 3));
}

PlayerReward Player::AddStar(uint32_t n) {
  star_counter_ += n;
  if (star_counter_ >= star_threshold_) {
    star_threshold_ += kStarThresholdIncrement;
    star_extend_count_++;

    // reward loop: EB...B|EB...B
    if (star_extend_count_ % kStarExtendLoop == 1) {
      left_++;
      return PlayerReward::Extend;
    }

    bomb_++;
    return PlayerReward::Bomb;
  }

  return PlayerReward::None;
}

void Player::ResetForContinue(int player_stock) {
  evade_sum_ = 0;
  left_ = player_stock;
  score_ = ((score_ % 10) + 1);
  star_counter_ = 0;
  star_threshold_ = kInitialStarThreshold;
  star_extend_count_ = 0;
  game_over_ = false;
}

PlayerProgress Player::CaptureProgress() const {
  return {
      .score = score_,
      .pending_score = dscore_,
      .graze_sum = evade_sum_,
      .pending_graze_score = evadesc_,
      .star_counter = star_counter_,
      .star_threshold = star_threshold_,
      .graze_count = evade_,
      .graze_wait = evade_c_,
      .power_progress = exp2_,
      .miss_count = miss_count_,
      .bomb_used = bomb_used_,
      .deathbomb_count = deathbomb_count_,
      .player_type = std::to_underlying(Type()),
      .power = exp_,
      .bombs = bomb_,
      .lives = left_,
      .credits = credit_,
      .star_extend_count = star_extend_count_,
      .initial_bomb_stock = initial_bomb_stock_,
  };
}

void Player::RestoreProgress(const PlayerProgress &progress) {
  SelectType(
      static_cast<PlayerType>(std::min<uint8_t>(progress.player_type, 2)));
  PrepareNextStage();

  score_ = progress.score;
  dscore_ = progress.pending_score;
  evade_sum_ = progress.graze_sum;
  evadesc_ = progress.pending_graze_score;
  star_counter_ = progress.star_counter;
  star_threshold_ = progress.star_threshold;
  evade_ = progress.graze_count;
  evade_c_ = progress.graze_wait;
  exp2_ = progress.power_progress;
  miss_count_ = progress.miss_count;
  bomb_used_ = progress.bomb_used;
  deathbomb_count_ = progress.deathbomb_count;
  exp_ = progress.power;
  bomb_ = progress.bombs;
  left_ = progress.lives;
  credit_ = progress.credits;
  star_extend_count_ = progress.star_extend_count;
  initial_bomb_stock_ = progress.initial_bomb_stock;
  game_over_ = false;
}
