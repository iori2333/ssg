///
/// PlayerTypes - Player type definitions
///

#pragma once

#include <cstdint>

// [Player class]

struct Player {
  // --- Coordinates ---
  int x, y;     // Current display coordinates
  int vx, vy;   // Option offset
  int opx, opy; // Current option base coordinates

  // --- Score ---
  int64_t score;  // Score counter
  int64_t dscore; // Score increment

  // --- Graze ---
  uint32_t evade_sum; // Total graze count
  int evadesc;        // Graze score
  uint16_t evade;     // Graze count
  uint16_t evade_c;   // Consecutive graze remaining tolerance

  // --- Status ---
  char v;              // Cactus base movement speed (later multiply by 64~45)
  uint8_t weapon;      // Thorn type
  uint8_t exp;         // Cactus experience?
  uint8_t bomb;        // Bomb count
  uint8_t left;        // Remaining cactus count
  uint8_t credit;      // Remaining credits
  uint16_t miss_count;        // Miss count
  uint16_t bomb_used;         // Bomb usage count
  uint16_t deathbomb_count;   // Deathbomb success count

  uint8_t GrpID; // Graphic to display

  // --- Timer/State ---
  uint16_t bomb_time;   // Bomb wait timer
  uint16_t exp2;        // Experience gain suppression
  uint16_t muteki;         // Invincibility flag (0:off !0:invincibility timer)
  uint16_t deathbomb_time; // Deathbomb window timer (0=inactive)
  uint16_t lay_time;       // Laser fire timing
  uint8_t lay_grp;      // Laser graphic
  uint8_t toge_time;    // Thorn fire timing
  uint8_t toge_ex;      // Thorn fire special variable
  uint8_t ShiftCounter; // Hold-to-move-slowly counter

  bool bGameOver; // Game over flag
  bool BuzzSound; // Prevent continuous graze sound

  // --- Methods ---
  void Draw();             // Player draw (MaidDraw)
  void DrawStatus() const; // Status draw (StateDraw)
  void Update();           // Per-frame update (MaidMove)
  void Initialize();       // Initialize (MaidSet)
  void PrepareNextStage(); // Next stage preparation (MaidNextStage)
  void OnHit();                     // Hit handler (deathbomb entry point)
  void OnDeath(bool play_se = true); // Death handler (MaidDead)

  void AddEvade(uint8_t n); // Graze gauge increase (evade_add)
  void AddEvadeEx(int x, int y,
                  uint8_t n);   // Graze effect from specified coordinates (evade_addEx)
  void AddScore(int sc);        // Score addition (score_add)
  void DrawWideBomb() const;    // Wide bomb draw (WideBombDraw)
  void PowerUp(uint8_t damage); // Power-up processing
  [[nodiscard]] uint8_t GetLaserDeg() const; // Get laser angle
  static uint8_t GetRightLaserDeg(uint8_t LaserDeg, int i);
  static uint8_t GetLeftLaserDeg(uint8_t LaserDeg, int i);

private:
  void DrawLaserBomb() const; // Laser bomb draw
  static uint8_t GetLeftOrRightLaserDeg(uint8_t LaserDeg, int i);
};

// Backward-compatible alias
// (MAID alias removed — use Player directly)
