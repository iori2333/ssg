///
/// ENEMY.h - Enemy management and spawn control
///

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

// [Revision history]

// 2000/10/17 : Fixed referencing wrong variable for play rank (was referencing
// ConfigDat) 2000/03/22 : Added LLaser processing (actual firing done by direct
// assignment to struct) 2000/02/25 : Added enemy hit-check function
// enemy_damage() 2000/02/22 : Changed enemy clipping range.

#include "bullet/bullet.h"
#include "bullet/laser/reflect.h"
#include "enemy/ecl/ecl.h"

// Enemy constants
inline constexpr uint16_t ENEMY_MAX = 50; // Maximum number of enemies

// Enemy state flags
inline constexpr uint8_t EF_DRAW = 0x01;   // Whether to draw the enemy
inline constexpr uint8_t EF_CLIP = 0x02;   // Whether to delete when off-screen
inline constexpr uint8_t EF_DAMAGE = 0x04; // Whether the enemy can take damage
inline constexpr uint8_t EF_HITSB = 0x08;  // Whether enemy collides with player
inline constexpr uint8_t EF_RLCHG =
    0x10; // Whether to enable ECL horizontal flip

enum class EnemyActorState : uint8_t {
  Active,
  Exploding,
  PendingRemoval,
};

inline constexpr int ENEMY_BOMB_SPD = 4;

// Homing constants
inline constexpr int HOMING_DUMMY = 500_px; // Dummy value when not homing

// Animation constants
inline constexpr std::size_t ENEMY_ANIMATION_MAX = 50;
inline constexpr std::size_t ENEMY_ANIMATION_FRAME_MAX = 16;
inline constexpr uint8_t ANM_NORM = 0x00; // Normal animation
inline constexpr uint8_t ANM_DEG = 0x01;  // Animate by angle
inline constexpr uint8_t ANM_STOP = 0x02; // Stop at final pattern

struct EclInterruptState {
  std::optional<size_t> target;
  uint32_t threshold = 0;
};

struct EclScriptState {
  size_t position = 0;
  size_t return_position = 0;
  uint32_t interrupt_timer = 0;
  std::array<uint32_t, ECL_REGISTER_COUNT> registers{};
  std::array<EclInterruptState, ECL_INTERRUPT_COUNT> interrupts{};
  uint16_t loop_counter = 0;
  uint16_t wait_counter = 0;
};

struct EnemyAnimation {
  uint8_t mode;    // Animation mode
  uint8_t n;       // Number of animation patterns
  PIXEL_SIZE size; // Image width, image height
  PIXEL_LTRB ptn[ENEMY_ANIMATION_FRAME_MAX];

  void SetSheet(PIXEL_POINT topleft, uint8_t frame_count, PIXEL_SIZE frame_size,
                uint8_t animation_mode) {
    size = frame_size;
    n = static_cast<uint8_t>(
        std::min<std::size_t>(frame_count, ENEMY_ANIMATION_FRAME_MAX));
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
    SetSquareSheet(topleft, static_cast<uint8_t>(ENEMY_ANIMATION_FRAME_MAX),
                   frame_size, ANM_DEG);
  }
};

using EnemyAnimationSet = std::array<EnemyAnimation, ENEMY_ANIMATION_MAX>;

struct PlayerAttack;

// Shared actor core used by regular enemies, bosses, and boss parts.
struct EnemyActor {
  void BeginExplosion();
  [[nodiscard]] bool IsHitBy(const PlayerAttack &attack) const;
  void UpdateAnimation(const EnemyAnimationSet &animations);

  EnemyActorState state = EnemyActorState::Active;

  WORLD_COORD x{}, y{}; // Display coordinates
  int vx{}, vy{};       // Velocity (x,y) components (x64)

  int v{}; // Velocity component (x64)

  uint32_t hp{};    // Remaining HP (too large?)
  uint32_t item{};  // Used for items etc.?
  uint32_t count{}; // Multipurpose frame counter

  uint32_t score{}; // Score (time-based score variation?)
  uint32_t graze_score{};

  EclScriptState script{};

  uint16_t hitbox_half_width{};
  uint16_t hitbox_half_height{};
  uint16_t animation_frame{};

  uint8_t d{};   // Direction angle 256
  int8_t vd{};   // Angular velocity 128
  uint8_t amp{}; // Amplitude 256
  uint8_t animation{};
  uint8_t damage_animation{};
  int8_t animation_speed{};
  uint8_t damage_flash{};

  uint8_t flag{}; // Enemy state flags (resize as needed)
  uint8_t auto_fire_frame{};
  uint8_t auto_fire_interval{};

  uint8_t long_laser_count{};

  BulletCommand bullet_command{};
  LaserCommand laser_command{};
};
