/// Shared enemy actor state, animation, and script execution state.

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "bullet/fire_state.h"
#include "enemy/ecl/ecl.h"
#include "gfx/coords.h"
#include "util/enum_flags.h"

inline constexpr std::size_t kEnemyCapacity = 50;

enum class EnemyActorFlags : uint8_t {
  None = 0,
  Draw = 1 << 0,
  KeepOutsidePlayfield = 1 << 1,
  Damageable = 1 << 2,
  CollidesWithPlayer = 1 << 3,
  HorizontalMirror = 1 << 4,
  HAS_BITFLAG_OPERATORS,
};

enum class EnemyActorState : uint8_t {
  Active,
  Exploding,
  PendingRemoval,
};

inline constexpr int kEnemyExplosionSpeed = 4;

// Homing constants
inline constexpr int kNoHomingDistance = 500_px;

// Animation constants
inline constexpr std::size_t kEnemyAnimationCapacity = 50;
inline constexpr std::size_t kEnemyAnimationFrameCapacity = 16;

enum class EnemyAnimationMode : uint8_t {
  Loop,
  Directional,
  StopAtEnd,
};

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
  EnemyAnimationMode mode = EnemyAnimationMode::Loop;
  uint8_t n = 0;
  PIXEL_SIZE size{};
  PIXEL_LTRB ptn[kEnemyAnimationFrameCapacity]{};

  void SetSheet(PIXEL_POINT topleft, uint8_t frame_count, PIXEL_SIZE frame_size,
                EnemyAnimationMode animation_mode) {
    size = frame_size;
    n = static_cast<uint8_t>(
        std::min<std::size_t>(frame_count, kEnemyAnimationFrameCapacity));
    mode = animation_mode;

    for (uint8_t frame = 0; frame < n; ++frame) {
      ptn[frame] = PIXEL_LTWH{topleft.x, topleft.y, frame_size.w, frame_size.h};
      topleft.x += frame_size.w;
    }
  }

  void SetSquareSheet(PIXEL_POINT topleft, uint8_t frame_count,
                      PIXEL_COORD frame_size,
                      EnemyAnimationMode animation_mode) {
    SetSheet(topleft, frame_count, {.w = frame_size, .h = frame_size},
             animation_mode);
  }

  void SetDirectionalSheet(PIXEL_POINT topleft, PIXEL_COORD frame_size) {
    SetSquareSheet(topleft, static_cast<uint8_t>(kEnemyAnimationFrameCapacity),
                   frame_size, EnemyAnimationMode::Directional);
  }
};

using EnemyAnimationSet = std::array<EnemyAnimation, kEnemyAnimationCapacity>;

struct PlayerAttack;
enum class ItemKind : uint8_t;

// Shared actor core used by regular enemies, bosses, and boss parts.
struct EnemyActor {
  void BeginExplosion();
  [[nodiscard]] bool HasFlag(EnemyActorFlags flag) const {
    return std::to_underlying(flags & flag) != 0;
  }
  void SetFlag(EnemyActorFlags flag, bool enabled) {
    EnumFlagSet(flags, flag, static_cast<uint8_t>(enabled));
  }
  [[nodiscard]] bool IsHitBy(const PlayerAttack &attack) const;
  void UpdateAnimation(const EnemyAnimationSet &animations);

  EnemyActorState state = EnemyActorState::Active;

  WORLD_COORD x{}, y{}; // Display coordinates
  int vx{}, vy{};       // Velocity (x,y) components (x64)

  int v{}; // Velocity component (x64)

  uint32_t hp{}; // Remaining HP (too large?)
  ItemKind item{};
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

  EnemyActorFlags flags = EnemyActorFlags::None;
  uint8_t auto_fire_frame{};
  uint8_t auto_fire_interval{};

  uint8_t long_laser_count{};

  EclBulletState bullet_command{};
  EclLaserState laser_command{};
};
