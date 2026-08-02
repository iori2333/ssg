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

inline constexpr int kRespawnInvincibilityDuration = 300;
inline constexpr int kRespawnMovementThreshold = 150;
inline constexpr int kBombEndInvincibilityDuration = 60;
inline constexpr int kPracticeHitInvincibilityDuration = 30;
inline constexpr int kDeathbombWindow = 12;
inline constexpr int kDeathbombCost = 2;

inline constexpr int kGrazeWaitMax = 256;
inline constexpr int kGrazeWaitIncrement = 2;

inline constexpr auto kShotCycleStart = 18;
inline constexpr auto kMainShotFrame = 6;
inline constexpr auto kSubShotFrame = 9;

inline constexpr auto kInitialStarThreshold = 150;
inline constexpr auto kStarThresholdIncrement = 150;
inline constexpr auto kStarExtendLoop = 3;
inline constexpr auto kStarCollectLine = 120_px;
inline constexpr auto kStarCollectGrazeWait = 128;

class EnemyManager;
class EffectManager;
struct GameSession;

namespace audio {
class AudioSystem;
}

namespace stage {
class StageSession;
}

enum class PlayerReward : uint8_t { None, Bomb, Extend };

struct PlayerUpdateResult {
  InputBits effective_input;
  bool clear_bullets;
  bool game_over;
};

struct PlayerProgress {
  int64_t score = 0;
  int64_t pending_score = 0;
  int graze_sum = 0;
  int pending_graze_score = 0;
  int star_counter = 0;
  int star_threshold = 0;
  int graze_count = 0;
  int graze_wait = 0;
  int power_progress = 0;
  int miss_count = 0;
  int bomb_used = 0;
  int deathbomb_count = 0;
  PlayerType player_type = PlayerType::Wide;
  int power = 0;
  int bombs = 0;
  int lives = 0;
  int credits = 0;
  int star_extend_count = 0;
  int initial_bomb_stock = 0;
};

class Player {
  enum class LifeState : uint8_t { Active, DeathbombWindow, Respawning };
  enum class BombTrigger : uint8_t { Manual, Deathbomb };

public:
  Player(EffectManager &effects, GameSession &session,
         stage::StageSession &stage, audio::AudioSystem &audio);
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
  void SetFocusHitboxVisible(bool visible) { focus_hitbox_visible_ = visible; }
  void Draw();
  void DrawBombBackground() const;
  void DrawDebugHitbox() const;
  void DrawProjectiles() const;
  [[nodiscard]] PlayerUpdateResult Update(EnemyManager &enemies,
                                          InputBits input);
  void Initialize(int player_stock, int bomb_stock);
  void PrepareNextStage();
  void OnHit();

  // --- Graze / score / power ---
  void AddEvade(int n);
  void AddEvadeEx(int x, int y, int n);
  void AddScore(int sc);
  void PowerUp(int damage);

  // --- Read-only accessors ---
  [[nodiscard]] int X() const { return x_; }
  [[nodiscard]] int Y() const { return y_; }
  [[nodiscard]] int OpX() const { return opx_; }
  [[nodiscard]] int OpY() const { return opy_; }
  [[nodiscard]] int64_t Score() const { return score_; }
  [[nodiscard]] PlayerType Type() const { return loadout_->Type(); }
  [[nodiscard]] int HitRadius() const { return loadout_->HitRadius(); }
  [[nodiscard]] int Power() const { return exp_; }
  [[nodiscard]] int Bombs() const { return bomb_; }
  [[nodiscard]] int Lives() const { return left_; }
  [[nodiscard]] int Credits() const { return credit_; }
  [[nodiscard]] int MissCount() const { return miss_count_; }
  [[nodiscard]] int BombUsed() const { return bomb_used_; }
  [[nodiscard]] int DeathbombCount() const { return deathbomb_count_; }
  [[nodiscard]] int GrazeCount() const { return evade_; }
  [[nodiscard]] int GrazeSum() const { return evade_sum_; }
  [[nodiscard]] int GrazeWaitTime() const { return evade_c_; }
  [[nodiscard]] int StarCounter() const { return star_counter_; }
  [[nodiscard]] int StarThreshold() const { return star_threshold_; }
  [[nodiscard]] PracticeMode Practice() const { return practice_mode_; }
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
  void SetPower(int p) { exp_ = p; }
  void SetLives(int n) { left_ = n; }
  void SetCredits(int c) { credit_ = c; }
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
  [[nodiscard]] PlayerReward AddStar(int n);
  [[nodiscard]] PlayerProgress CaptureProgress() const;
  void RestoreProgress(const PlayerProgress &progress);

  // Loadouts emit projectiles through the owning Player.
  void SpawnShot(const PlayerShotSpawnInfo &si);

private:
  std::unique_ptr<PlayerLoadout> loadout_;
  EffectManager &effects_;
  audio::AudioSystem &audio_;

  [[nodiscard]] static bool IsMainShotFrame(int time);
  [[nodiscard]] bool IsSubShotFrame(int time) const;
  void DrawFocusHitbox() const;
  [[nodiscard]] int HitRadiusPixels() const;
  void UpdateStatus();
  InputBits PrepareInput(InputBits input);
  void UpdateMovement(InputBits input);
  void UpdateOptionPosition(int movement_x, int movement_y);
  void UpdateProjectiles(EnemyManager &enemies);
  void UpdateWeapons(EnemyManager &enemies, InputBits input);
  bool ActivateBomb(BombTrigger trigger);
  void EnterDeathbombWindow();
  void PlayHitFeedback() const;
  void CommitDeath();

  // --- Coordinates ---
  int x_ = 0, y_ = 0;
  int opx_ = 0, opy_ = 0;
  int option_lag_x_ = 0, option_lag_y_ = 0;

  // --- Score ---
  int64_t score_ = 0;
  int64_t dscore_ = 0;

  // --- Graze ---
  int evade_sum_ = 0;
  int evadesc_ = 0;
  int evade_ = 0;
  int evade_c_ = 0;

  // --- Star counter ---
  int star_counter_ = 0;
  int star_threshold_ = 0;
  int star_extend_count_ = 0;

  // --- Status ---
  int exp_ = 0;
  int bomb_ = 0;
  int left_ = 0;
  int credit_ = 0;
  int miss_count_ = 0;
  int bomb_used_ = 0;
  int deathbomb_count_ = 0;

  int grp_id_ = 0;

  // --- Timer/State ---
  int bomb_time_ = 0;
  int exp2_ = 0;
  int invincibility_time_ = 0;
  int deathbomb_time_ = 0;
  int toge_time_ = 0;
  int shift_counter_ = 0;

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
  bool focus_hitbox_visible_ = true;
  int initial_bomb_stock_ = 0;

  // --- Shot pool ---
  util::ObjectPool<PlayerShot, kPlayerShotCapacity> maid_tama_;
};
