///
/// ENEMY.h - Enemy management and spawn control
///

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

// [Revision history]

// 2000/10/17 : Fixed referencing wrong variable for play rank (was referencing
// ConfigDat) 2000/03/22 : Added LLaser processing (actual firing done by direct
// assignment to struct) 2000/02/25 : Added enemy hit-check function
// enemy_damage() 2000/02/22 : Changed enemy clipping range.

#include "ecl.h"

#include "bullet/bullet.h"
#include "bullet/laser/reflect.h"
#include "sys/buffer.h"

// Enemy constants
inline constexpr uint16_t ENEMY_MAX = 50; // Maximum number of enemies

// Enemy state flags
inline constexpr uint8_t EF_DRAW = 0x01;   // Whether to draw the enemy
inline constexpr uint8_t EF_CLIP = 0x02;   // Whether to delete when off-screen
inline constexpr uint8_t EF_DAMAGE = 0x04; // Whether the enemy can take damage
inline constexpr uint8_t EF_HITSB = 0x08;  // Whether enemy collides with player
inline constexpr uint8_t EF_RLCHG =
    0x10; // Whether to enable ECL horizontal flip
inline constexpr uint8_t EF_BOMB = 0x20;   // Enemy is exploding
inline constexpr uint8_t EF_DELETE = 0x40; // Delete enemy this frame

inline constexpr int ENEMY_BOMB_SPD = 4;

// Homing constants
inline constexpr int HOMING_DUMMY = (500 * 64); // Dummy value when not homing

// Animation constants
inline constexpr uint8_t ANIME_MAX = 50;    // Number of animation types
inline constexpr uint8_t ANIMEPTN_MAX = 16; // Maximum animation patterns
inline constexpr uint8_t ANM_NORM = 0x00;   // Normal animation
inline constexpr uint8_t ANM_DEG = 0x01;    // Animate by angle
inline constexpr uint8_t ANM_STOP = 0x02;   // Stop at final pattern

struct EclInterruptState {
  std::optional<size_t> target;
  uint32_t threshold = 0;
};

struct EclRuntime {
  size_t position = 0;
  size_t return_position = 0;
  uint32_t interrupt_timer = 0;
  std::array<uint32_t, ECL_REGISTER_COUNT> registers{};
  std::array<EclInterruptState, ECL_INTERRUPT_COUNT> interrupts{};
  uint16_t loop_counter = 0;
  uint16_t wait_counter = 0;
};

// Shared actor core used by regular enemies, bosses, and boss parts.
struct EnemyActor {
  WORLD_COORD x, y; // Display coordinates
  int vx, vy;       // Velocity (x,y) components (x64)

  int v; // Velocity component (x64)

  uint32_t hp;    // Remaining HP (too large?)
  uint32_t item;  // Used for items etc.?
  uint32_t count; // Multipurpose frame counter

  uint32_t score;   // Score (time-based score variation?)
  uint32_t evscore; // Graze score

  EclRuntime script;

  uint16_t g_width;  // Graphic width /2*64 (also used for hit detection)
  uint16_t g_height; // Graphic height /2*64 (same as above)
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
};

struct ANIME_DATA {
  uint8_t mode;                 // Animation mode
  uint8_t n;                    // Number of animation patterns
  PIXEL_SIZE size;              // Image width, image height
  PIXEL_LTRB ptn[ANIMEPTN_MAX]; // Rectangular areas for animation

  void SetSheet(PIXEL_POINT topleft, uint8_t frame_count, PIXEL_SIZE frame_size,
                uint8_t animation_mode) {
    size = frame_size;
    n = std::min(frame_count, ANIMEPTN_MAX);
    mode = animation_mode;

    for (uint8_t frame = 0; frame < n; ++frame) {
      ptn[frame] = PIXEL_LTWH{topleft.x, topleft.y, frame_size.w, frame_size.h};
      topleft.x += frame_size.w;
    }
  }

  void SetSquareSheet(PIXEL_POINT topleft, uint8_t frame_count,
                      PIXEL_COORD frame_size, uint8_t animation_mode) {
    SetSheet(topleft, frame_count, {.w = frame_size, .h = frame_size},
             animation_mode);
  }

  void SetDirectionalSheet(PIXEL_POINT topleft, PIXEL_COORD frame_size) {
    SetSquareSheet(topleft, ANIMEPTN_MAX, frame_size, ANM_DEG);
  }
};

using EnemyAnimationSet = std::array<ANIME_DATA, ANIME_MAX>;
