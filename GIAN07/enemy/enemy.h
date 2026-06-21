///
/// ENEMY.h - Enemy management and spawn control
///

#pragma once

// [Revision history]

// 2000/10/17 : Fixed referencing wrong variable for play rank (was referencing ConfigDat)
// 2000/03/22 : Added LLaser processing (actual firing done by direct assignment to struct)
// 2000/02/25 : Added enemy hit-check function enemy_damage()
// 2000/02/22 : Changed enemy clipping range.

#include "bullet/bullet.h"
#include "bullet/laser.h"
#include "ecl.h"
#include "game/buffer.h"

// Enemy constants
inline constexpr uint16_t ENEMY_MAX = 50; // Maximum number of enemies

// Enemy state flags
inline constexpr uint8_t EF_DRAW = 0x01;   // Whether to draw the enemy
inline constexpr uint8_t EF_CLIP = 0x02;   // Whether to delete when off-screen
inline constexpr uint8_t EF_DAMAGE = 0x04; // Whether the enemy can take damage
inline constexpr uint8_t EF_HITSB = 0x08;  // Whether enemy collides with player
inline constexpr uint8_t EF_RLCHG = 0x10;  // Whether to enable ECL horizontal flip
inline constexpr uint8_t EF_BOMB = 0x20;   // Enemy is exploding
inline constexpr uint8_t EF_DELETE = 0x40; // Delete enemy this frame

inline constexpr int ENEMY_BOMB_SPD = 4;

// Homing constants
inline constexpr int HOMING_DUMMY =
    (500 * 64); // Dummy value when not homing

// Animation constants
inline constexpr uint8_t ANIME_MAX = 50;    // Number of animation types
inline constexpr uint8_t ANIMEPTN_MAX = 16; // Maximum animation patterns
inline constexpr uint8_t ANM_NORM = 0x00;   // Normal animation
inline constexpr uint8_t ANM_DEG = 0x01;    // Animate by angle
inline constexpr uint8_t ANM_STOP = 0x02;   // Stop at final pattern

// Interrupt vector structure
struct InterruptVector {
  uint32_t vect; // Interrupt vector (0 if disabled)
  int value;     // Comparison value
};

// Enemy data structure
struct EnemyData {
  WORLD_COORD x, y; // Display coordinates
  int vx, vy;       // Velocity (x,y) components (x64)

  int v; // Velocity component (x64)

  uint32_t hp;        // Remaining HP (too large?)
  uint32_t item;      // Used for items etc.?
  uint32_t cmd;       // ECL command absolute address (major change from DOS version)
  uint32_t count;     // Multipurpose frame counter
  uint32_t call_addr; // Address to jump to after RET execution

  uint32_t score;   // Score (time-based score variation?)
  uint32_t evscore; // Graze score

  uint32_t IntTimer; // Interrupt timer

  uint32_t GR[ECLREG_MAX];           // Variable registers
  InterruptVector Vect[ECLVECT_MAX]; // Interrupt vectors

  uint16_t g_width;  // Graphic width /2*64 (also used for hit detection)
  uint16_t g_height; // Graphic height /2*64 (same as above)
  uint16_t rep_c;    // REP instruction counter
  uint16_t cmd_c;    // Current command repeat count
  uint16_t anm_c;    // Animation counter

  uint8_t d;         // Direction angle 256
  char vd;           // Angular velocity 128
  uint8_t amp;       // Amplitude 256
  uint8_t anm_ptn;   // Current animation pattern
  uint8_t anm_ptnEx; // Animation pattern while damaged
  char anm_sp;       // Animation speed
  uint8_t IsDamaged; // Whether damaged

  uint8_t flag;   // Enemy state flags (resize as needed)
  uint8_t tama_c; // Bullet fire counter
  uint8_t t_rep;  // Bullet fire interval

  uint8_t LLaserRef; // Thick laser reference count

  BulletCommand t_cmd; // Bullet fire command
  LaserCommand l_cmd;  // Laser fire command

  // --- Methods ---
  void Draw() const;
  void UpdateAnimation(); // EnemyAnimeMove()
};

// Backward compatibility aliases
// (ENEMY_DATA alias removed — use EnemyData directly)
// (INT_VECTOR alias removed — use InterruptVector directly)

struct ANIME_DATA {
  uint8_t mode;                 // Animation mode
  uint8_t n;                    // Number of animation patterns
  PIXEL_SIZE size;              // Image width, image height
  PIXEL_LTRB ptn[ANIMEPTN_MAX]; // Rectangular areas for animation

  template <uint8_t Count>
  void SetSheet(PIXEL_POINT topleft, PIXEL_SIZE size, uint8_t mode) {
    static_assert(Count <= ANIMEPTN_MAX);

    this->size = size;
    this->n = Count;
    this->mode = mode;

    for (auto i = decltype(Count){0}; i < Count; i++) {
      ptn[i] = PIXEL_LTWH{topleft.x, topleft.y, size.w, size.h};
      topleft.x += size.w;
    }
  }

  template <uint8_t Count, PIXEL_COORD Size>
  void SetSheet(PIXEL_POINT topleft, uint8_t mode) {
    SetSheet<Count>(topleft, {Size, Size}, mode);
  }

  template <PIXEL_COORD Size> void SetSheetDeg(PIXEL_POINT topleft) {
    SetSheet<16>(topleft, {.w = Size, .h = Size}, ANM_DEG);
  }
};

// Enemy variables
// Access directly via Enemies.entities, Enemies.indices, Enemies.count, Enemies.anime

// Enemy control functions
// Backward-compatible inline wrapper moved to enemy_manager.h
// Implementation migrated to EnemyManager methods
