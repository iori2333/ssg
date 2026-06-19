///
/// Player - Player (maid) ship: state, movement, attack, and bombs.
///
/// All state is private; external code interacts through accessors
/// and action methods.  Attack forms are modelled as WeaponForm
/// strategy objects.
///

#pragma once

#include "game/cast.h"
#include "player_shot.h"
#include <array>
#include <cstdint>
#include <memory>

// [ Constants ]

inline constexpr int VIVDEAD_VAL = 300;   // Viv death time
inline constexpr int VIVMUTEKI_VAL = 180; // Viv invincibility time

inline constexpr int MAID_MOVE_DISABLE_TIME =
    (250 - 100); // Move-disabled duration

inline constexpr int BOMBMUTEKI_VAL = 60; // Bomb-end invincibility
inline constexpr int SBOPT_DX = 26;       // Option offset (not x64)

inline constexpr int DEATHBOMB_WINDOW =
    12; // Deathbomb input window (base, Lunatic)

inline constexpr int EVADETIME_MAX = 256; // Max graze wait time

inline constexpr int SSP_WIDE = (64 * 9);
inline constexpr int SSP_HOMING = (64 * 9);
inline constexpr int SSP_LASER = (64 * 13);

inline constexpr auto WIDE_BOMB_TIME = (60 * 4);
inline constexpr auto HOMING_BOMB_TIME = (60 * 3);
inline constexpr auto LASER_BOMB_TIME = (60 * 2);

inline constexpr auto MAID_TAMA_START = 18;
inline constexpr auto MAID_MAIN_SHOT = 6;
inline constexpr auto MAID_SUB_SHOT = 9;

// [ Forward declarations ]
class WeaponForm;
class WideForm;
class WideFocusForm;
class HomingForm;
class HomingFocusForm;
class LaserForm;
class LaserFocusForm;

// [ Player class ]

class Player {
 public:
  Player();
  ~Player();
  Player(const Player& other);
  Player& operator=(const Player& other);

  // --- Lifecycle ---
  void Draw();
  void DrawStatus() const;
  void Update();
  void Initialize();
  void PrepareNextStage();
  void OnHit();
  void OnDeath(bool play_se = true);

  // --- Graze / score / power ---
  void AddEvade(uint8_t n);
  void AddEvadeEx(int x, int y, uint8_t n);
  void AddScore(int sc);
  void DrawWideBomb() const;
  void PowerUp(uint8_t damage);

  // --- Shot system ---
  void SetMaidShot();
  void MoveMaidShot();
  void DrawMaidShot();
  void SetMaidShotIndices();
  static void SetMLaser(uint16_t time);

  // --- Laser angle ---
  [[nodiscard]] uint8_t GetLaserDeg() const;
  static uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
  static uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);

  // --- Read-only accessors ---
  int X() const { return x_; }
  int Y() const { return y_; }
  int OpX() const { return opx_; }
  int OpY() const { return opy_; }
  int64_t Score() const { return score_; }
  uint8_t Weapon() const { return weapon_; }
  uint8_t Power() const { return exp_; }
  uint8_t Bombs() const { return bomb_; }
  uint8_t Lives() const { return left_; }
  uint8_t Credits() const { return credit_; }
  uint16_t MissCount() const { return miss_count_; }
  uint16_t BombUsed() const { return bomb_used_; }
  uint16_t DeathbombCount() const { return deathbomb_count_; }
  uint16_t GrazeCount() const { return evade_; }
  uint32_t GrazeSum() const { return evade_sum_; }
  uint32_t StarCounter() const { return star_counter_; }
  uint32_t StarThreshold() const { return star_threshold_; }
  bool IsInvincible() const { return muteki_ != 0; }
  bool IsBombActive() const { return bomb_time_ != 0; }
  bool IsGameOver() const { return game_over_; }
  uint16_t ShotCount() const { return maid_tama_now_; }

  // --- Setters / action methods ---
  void SetWeapon(uint8_t w) { weapon_ = w; }
  void RotateWeapon(int dir);
  void SetPower(uint8_t p) { exp_ = p; }
  void SetLives(uint8_t n) { left_ = n; }
  void SetBombs(uint8_t b) { bomb_ = b; }
  void SetCredits(uint8_t c) { credit_ = c; }
  void UseCredit() { credit_--; }
  void SetScore(int64_t s) { score_ = s; }
  void ResetForContinue();
  void ClearInvincibility() { muteki_ = 0; }
  void ClearLaserState() { lay_time_ = 0; lay_grp_ = 0; }
  void SetPosition(int nx, int ny) { x_ = nx; y_ = ny; }
  void PickupBomb() { bomb_++; }
  void PickupExtend() { left_++; }
  void AddStar(uint32_t n);
  void ApplyReplayState(uint8_t weapon, uint8_t exp, uint8_t left,
                        uint8_t bombs);

  // --- Shot pool helper (used by WeaponForm subclasses) ---
  void SpawnShot_();

 private:
  friend class WeaponForm;
  friend class WideForm;
  friend class WideFocusForm;
  friend class HomingForm;
  friend class HomingFocusForm;
  friend class LaserForm;
  friend class LaserFocusForm;

  // --- Weapon form strategy objects ---
  std::array<std::unique_ptr<WeaponForm>, 6> forms_;
  WeaponForm* BaseForm_() const;
  WeaponForm* ActiveForm_() const;

  // --- Shot helpers (internal) ---
  bool IsMainShotFrame_(uint16_t t) const;
  bool IsSubShotFrame_(uint16_t t) const;

  void DrawLaserBomb_() const;
  static uint8_t GetLeftOrRightLaserDeg_(uint8_t LaserDeg, int i);

  // --- Coordinates ---
  int x_ = 0, y_ = 0;
  int vx_ = 0, vy_ = 0;
  int opx_ = 0, opy_ = 0;

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
  char v_ = 0;
  uint8_t weapon_ = 0;
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
  uint16_t muteki_ = 0;
  uint16_t deathbomb_time_ = 0;
  uint16_t lay_time_ = 0;
  uint8_t lay_grp_ = 0;
  uint8_t toge_time_ = 0;
  uint8_t toge_ex_ = 0;
  uint8_t shift_counter_ = 0;

  bool game_over_ = false;
  bool buzz_sound_ = false;

  // --- Shot pool ---
  std::array<Bullet, MAIDTAMA_MAX> maid_tama_{};
  std::array<uint16_t, MAIDTAMA_MAX> maid_tama_ind_{};
  uint16_t maid_tama_now_ = 0;
};

// [ Global instance ]
extern Player Players;
