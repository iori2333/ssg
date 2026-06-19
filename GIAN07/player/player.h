///
/// Player - Player (maid) ship: state, movement, attack, and bombs.
///
/// Merges the former Player struct + PlayerManager into a single class.
/// Phase 1 of the refactor: fields remain public for mechanical migration.
///

#pragma once

#include "game/cast.h"
#include "player_shot.h"
#include <array>
#include <cstdint>

// [ Constants ]

// Cactus/viv constants
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

// [ Player class ]

class Player {
 public:
  // --- Coordinates ---
  int x = 0, y = 0;     // Current display coordinates
  int vx = 0, vy = 0;   // Option offset
  int opx = 0, opy = 0; // Current option base coordinates

  // --- Score ---
  int64_t score = 0;  // Score counter
  int64_t dscore = 0; // Score increment

  // --- Graze ---
  uint32_t evade_sum = 0; // Total graze count
  int evadesc = 0;        // Graze score
  uint16_t evade = 0;     // Graze count
  uint16_t evade_c = 0;   // Consecutive graze remaining tolerance

  // --- Star counter ---
  uint32_t star_counter = 0;     // Cumulative star count
  uint32_t star_threshold = 0;   // Next extend threshold
  uint8_t star_extend_count = 0; // Extends granted via stars

  // --- Status ---
  char v = 0;         // Cactus base movement speed (later multiply by 64~45)
  uint8_t weapon = 0; // Thorn type
  uint8_t exp = 0;    // Cactus experience?
  uint8_t bomb = 0;   // Bomb count
  uint8_t left = 0;   // Remaining cactus count
  uint8_t credit = 0; // Remaining credits
  uint16_t miss_count = 0;      // Miss count
  uint16_t bomb_used = 0;       // Bomb usage count
  uint16_t deathbomb_count = 0; // Deathbomb success count

  uint8_t GrpID = 0; // Graphic to display

  // --- Timer/State ---
  uint16_t bomb_time = 0;   // Bomb wait timer
  uint16_t exp2 = 0;        // Experience gain suppression
  uint16_t muteki = 0;      // Invincibility flag (0:off !0:invincibility timer)
  uint16_t deathbomb_time = 0; // Deathbomb window timer (0=inactive)
  uint16_t lay_time = 0;       // Laser fire timing
  uint8_t lay_grp = 0;      // Laser graphic
  uint8_t toge_time = 0;    // Thorn fire timing
  uint8_t toge_ex = 0;      // Thorn fire special variable
  uint8_t ShiftCounter = 0; // Hold-to-move-slowly counter

  bool bGameOver = false; // Game over flag
  bool BuzzSound = false; // Prevent continuous graze sound

  // --- Shot pool (formerly PlayerManager) ---
  std::array<Bullet, MAIDTAMA_MAX> maid_tama{};
  std::array<uint16_t, MAIDTAMA_MAX> maid_tama_ind{};
  uint16_t maid_tama_now = 0;

  // --- Ship lifecycle ---
  void Draw();             // Player draw (MaidDraw)
  void DrawStatus() const; // Status draw (StateDraw)
  void Update();           // Per-frame update (MaidMove)
  void Initialize();       // Initialize (MaidSet)
  void PrepareNextStage(); // Next stage preparation (MaidNextStage)
  void OnHit();            // Hit handler (deathbomb entry point)
  void OnDeath(bool play_se = true); // Death handler (MaidDead)

  // --- Graze / score / power ---
  void AddEvade(uint8_t n); // Graze gauge increase (evade_add)
  void AddEvadeEx(int x, int y,
                  uint8_t n);   // Graze effect from specified coordinates
  void AddScore(int sc);        // Score addition (score_add)
  void DrawWideBomb() const;    // Wide bomb draw (WideBombDraw)
  void PowerUp(uint8_t damage); // Power-up processing

  // --- Shot system (formerly PlayerManager) ---
  void SetMaidShot();
  void MoveMaidShot();
  void DrawMaidShot();
  void SetMaidShotIndices();
  static void SetMLaser(uint16_t time);

  // --- Laser angle ---
  [[nodiscard]] uint8_t GetLaserDeg() const;
  static uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
  static uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);

 private:
  void DrawLaserBomb() const; // Laser bomb draw
  static uint8_t GetLeftOrRightLaserDeg(uint8_t LaserDeg, int i);
};

// [ Global instance ]
extern Player Players;

// [ Backward-compatibility function wrappers (to be phased out) ]
inline void MaidDraw() { Players.Draw(); }
inline void StateDraw() { Players.DrawStatus(); }
inline void MaidMove() { Players.Update(); }
inline void MaidSet() { Players.Initialize(); }
inline void MaidNextStage() { Players.PrepareNextStage(); }
inline void MaidDead() { Players.OnDeath(); }
inline void MaidHit() { Players.OnHit(); }
inline void evade_add(uint8_t n) { Players.AddEvade(n); }
inline void evade_addEx(int x, int y, uint8_t n) {
  Players.AddEvadeEx(x, y, n);
}
inline void score_add(int sc) { Players.AddScore(sc); }
inline void WideBombDraw() { Players.DrawWideBomb(); }
inline void PowerUp(uint8_t damage) { Players.PowerUp(damage); }
inline uint8_t GetLaserDeg() { return Players.GetLaserDeg(); }
// Laser angle calculation (public because player_shot.cpp references it)
uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);
