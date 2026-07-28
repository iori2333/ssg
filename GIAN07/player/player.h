///
/// Player - Player state, lifecycle, movement, attacks, and projectiles.
///

#pragma once

#include <cstdint>
#include <memory>

#include "loadout/player_loadout.h"
#include "player_shot.h"

#include "gameplay/game_rules.h"
#include "gfx/coords.h"
#include "sys/input.h"
#include "util/object_pool.h"

// [ Constants ]

inline constexpr int RESPAWN_INVINCIBILITY_DURATION = 300;
inline constexpr int RESPAWN_MOVEMENT_THRESHOLD = 150;
inline constexpr int BOMB_END_INVINCIBILITY_DURATION = 60;
inline constexpr int PRACTICE_HIT_INVINCIBILITY_DURATION = 30;
inline constexpr int DEATHBOMB_WINDOW = 12; // Base window at Lunatic difficulty
inline constexpr uint8_t DEATHBOMB_COST = 2;

inline constexpr uint16_t EVADETIME_MAX = 256; // Max graze wait time
inline constexpr uint16_t EVADETIME_INCR = 2;  // Wait time increase value

inline constexpr auto MAID_TAMA_START = 18;
inline constexpr auto MAID_MAIN_SHOT = 6;
inline constexpr auto MAID_SUB_SHOT = 9;

inline constexpr auto STAR_THRESHOLD_INIT = 150;
inline constexpr auto STAR_THRESHOLD_INCR = 150;
inline constexpr auto STAR_EXTEND_LOOP = 3;
inline constexpr auto STAR_COLLECT_LINE = 120_px;
inline constexpr auto STAR_COLLECT_EVADETIME = 128;

inline constexpr auto BOMB_RANK_DECR = 25;
inline constexpr auto DEATH_RANK_DECR = 100;

class EnemyManager;
class EffectManager;
struct GameSession;

namespace stage {
class StageSession;
}

// [ Player class ]
enum class PlayerReward : uint8_t { NONE, BOMB, EXTEND };

struct PlayerUpdateResult {
  INPUT_BITS effective_input;
  bool clear_bullets;
  bool game_over;
};

struct PlayerProgress {
  int64_t score = 0;
  int64_t pending_score = 0;
  uint32_t graze_sum = 0;
  int32_t pending_graze_score = 0;
  uint32_t star_counter = 0;
  uint32_t star_threshold = 0;
  uint16_t graze_count = 0;
  uint16_t graze_wait = 0;
  uint16_t power_progress = 0;
  uint16_t miss_count = 0;
  uint16_t bomb_used = 0;
  uint16_t deathbomb_count = 0;
  uint8_t player_type = 0;
  uint8_t power = 0;
  uint8_t bombs = 0;
  uint8_t lives = 0;
  uint8_t credits = 0;
  uint8_t star_extend_count = 0;
  uint8_t initial_bomb_stock = 0;
};

class Player {
  enum class LifeState : uint8_t { Active, DeathbombWindow, Respawning };
  enum class BombTrigger : uint8_t { Manual, Deathbomb };

public:
  Player(EffectManager &effects, GameSession &session,
         stage::StageSession &stage);
  ~Player() = default;
  Player(const Player &other) = delete;
  Player(Player &&other) = delete;
  Player &operator=(const Player &other) = delete;
  Player &operator=(Player &&other) = delete;

  // --- Lifecycle ---
  void Configure(PracticeMode practice_mode, bool focus_while_firing) {
    practice_mode_ = practice_mode;
    focus_while_firing_ = focus_while_firing;
  }
  void Draw();
  void DrawBombBackground() const;
  void DrawDebugHitbox() const;
  void DrawProjectiles() const;
  [[nodiscard]] PlayerUpdateResult Update(EnemyManager &enemies,
                                          INPUT_BITS input);
  void Initialize(int player_stock, int bomb_stock);
  void PrepareNextStage();
  void OnHit();

  // --- Graze / score / power ---
  void AddEvade(uint8_t n);
  void AddEvadeEx(int x, int y, uint8_t n);
  void AddScore(int sc);
  void PowerUp(uint8_t damage);

  // --- Read-only accessors ---
  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] int Y() const { return y_; }
  [[nodiscard]] int OpX() const { return opx_; }
  [[nodiscard]] int OpY() const { return opy_; }
  [[nodiscard]] int64_t Score() const { return score_; }
  [[nodiscard]] PlayerType Type() const { return loadout_->Type(); }
  [[nodiscard]] int HitRadius() const { return loadout_->HitRadius(); }
  [[nodiscard]] uint8_t Power() const { return exp_; }
  [[nodiscard]] uint8_t Bombs() const { return bomb_; }
  [[nodiscard]] uint8_t Lives() const { return left_; }
  [[nodiscard]] uint8_t Credits() const { return credit_; }
  [[nodiscard]] uint16_t MissCount() const { return miss_count_; }
  [[nodiscard]] uint16_t BombUsed() const { return bomb_used_; }
  [[nodiscard]] uint16_t DeathbombCount() const { return deathbomb_count_; }
  [[nodiscard]] uint16_t GrazeCount() const { return evade_; }
  [[nodiscard]] uint32_t GrazeSum() const { return evade_sum_; }
  [[nodiscard]] uint16_t GrazeWaitTime() const { return evade_c_; }
  [[nodiscard]] uint32_t StarCounter() const { return star_counter_; }
  [[nodiscard]] uint32_t StarThreshold() const { return star_threshold_; }
  [[nodiscard]] bool IsInvincible() const {
    return invincibility_time_ != 0 ||
           life_state_ == LifeState::DeathbombWindow ||
           life_state_ == LifeState::Respawning;
  }
  [[nodiscard]] bool IsBombActive() const { return bomb_time_ != 0; }
  [[nodiscard]] bool IsMovementDisabled() const {
    return life_state_ == LifeState::Respawning;
  }

  // --- Setters / action methods ---
  void SelectType(PlayerType type);
  void RotateType(int dir);
  void SetPower(uint8_t p) { exp_ = p; }
  void SetLives(uint8_t n) { left_ = n; }
  void SetCredits(uint8_t c) { credit_ = c; }
  void UseCredit() { credit_--; }
  void ResetForContinue(int player_stock);
  void ClearInvincibility() {
    invincibility_time_ = 0;
    deathbomb_time_ = 0;
    life_state_ = LifeState::Active;
  }
  void ClearContinuousAttack() { loadout_->ClearContinuousAttack(); }
  void SetPosition(int nx, int ny) {
    x_ = nx;
    y_ = ny;
  }
  void PickupBomb() { bomb_++; }
  void PickupExtend() { left_++; }
  [[nodiscard]] PlayerReward AddStar(uint32_t n);
  [[nodiscard]] PlayerProgress CaptureProgress() const;
  void RestoreProgress(const PlayerProgress &progress);

  // Loadouts emit projectiles through the owning Player.
  void SpawnShot(const PlayerShotSpawnInfo &si);

private:
  std::unique_ptr<PlayerLoadout> loadout_;
  EffectManager &effects_;

  bool IsMainShotFrame_(uint16_t t) const;
  bool IsSubShotFrame_(uint16_t t) const;
  void DrawFocusHitbox_() const;
  [[nodiscard]] int HitRadiusPixels_() const;
  void UpdateStatus_();
  INPUT_BITS PrepareInput_(INPUT_BITS input);
  void UpdateMovement_(INPUT_BITS input);
  void UpdateOptionPosition_(int movement_x, int movement_y);
  void UpdateProjectiles_(EnemyManager &enemies);
  void UpdateWeapons_(EnemyManager &enemies, INPUT_BITS input);
  bool ActivateBomb_(BombTrigger trigger);
  void EnterDeathbombWindow_();
  void PlayHitFeedback_() const;
  void CommitDeath_();

  // --- Coordinates ---
  int x_ = 0, y_ = 0;
  int opx_ = 0, opy_ = 0;
  int option_lag_x_ = 0, option_lag_y_ = 0;

  // --- Score ---
  int64_t score_ = 0;
  int64_t dscore_ = 0;

  // --- Graze ---
  uint32_t evade_sum_ = 0;
  int evadesc_ = 0;
  uint16_t evade_ = 0;
  uint16_t evade_c_ = 0;

  // --- Star counter ---
  uint32_t star_counter_ = 0;
  uint32_t star_threshold_ = 0;
  uint8_t star_extend_count_ = 0;

  // --- Status ---
  uint8_t exp_ = 0;
  uint8_t bomb_ = 0;
  uint8_t left_ = 0;
  uint8_t credit_ = 0;
  uint16_t miss_count_ = 0;
  uint16_t bomb_used_ = 0;
  uint16_t deathbomb_count_ = 0;

  uint8_t grp_id_ = 0;

  // --- Timer/State ---
  uint16_t bomb_time_ = 0;
  uint16_t exp2_ = 0;
  uint16_t invincibility_time_ = 0;
  uint16_t deathbomb_time_ = 0;
  uint8_t toge_time_ = 0;
  uint8_t shift_counter_ = 0;

  bool game_over_ = false;
  bool buzz_sound_ = false;
  bool focused_ = false;
  bool clear_bullets_requested_ = false;
  bool auto_bomb_requested_ = false;
  LifeState life_state_ = LifeState::Respawning;

  GameSession &session_;
  stage::StageSession &stage_;
  PracticeMode practice_mode_ = PracticeMode::Off;
  bool focus_while_firing_ = false;
  uint8_t initial_bomb_stock_ = 0;

  // --- Shot pool ---
  ObjectPool<PlayerShot, kPlayerShotCapacity> maid_tama_{};
};
