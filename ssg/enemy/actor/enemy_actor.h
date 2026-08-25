/// Shared enemy actor state, animation, and script execution state.

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "bullet/fire_state.h"
#include "enemy/ecl/ecl.h"
#include "gfx/core/coords.h"
#include "gfx/core/rect.h"
#include "util/enum_flags.h"

inline constexpr std::size_t kEnemyCapacity = 50;

enum class EnemyActorFlags : uint8_t {
  None = 0,
  Draw = 1 << 0,
  KeepOutsidePlayfield = 1 << 1,
  Damageable = 1 << 2,
  CollidesWithPlayer = 1 << 3,
  HorizontalMirror = 1 << 4,
};

template <> inline constexpr bool util::EnableEnumFlags<EnemyActorFlags> = true;

enum class EnemyActorState : uint8_t {
  Active,
  Exploding,
  PendingRemoval,
};

inline constexpr int kEnemyExplosionSpeed = 4;

// Homing constants
inline constexpr auto kNoHomingDistance = 500_px;

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
  int threshold = 0;
};

struct EclScriptState {
  size_t position = 0;
  size_t return_position = 0;
  int interrupt_timer = 0;
  std::array<uint32_t, kEclRegisterCount> registers{};
  std::array<EclInterruptState, kEclInterruptCount> interrupts{};
  int loop_counter = 0;
  int wait_counter = 0;
  // Sequence-angle animation state. Per-actor so concurrently running scripts
  // (multiple enemies through the shared EclVm) do not overwrite each other.
  int sequence_angle = 0;
  int sequence_angle_delta = 0;
};

struct EnemyAnimation {
  EnemyAnimationMode mode = EnemyAnimationMode::Loop;
  std::size_t n = 0;
  PixelPoint size{};
  std::array<Rect, kEnemyAnimationFrameCapacity> ptn{};

  void SetSheet(PixelPoint topleft, std::size_t frame_count,
                PixelPoint frame_size, EnemyAnimationMode animation_mode) {
    size = frame_size;
    n = std::min<std::size_t>(frame_count, kEnemyAnimationFrameCapacity);
    mode = animation_mode;

    for (std::size_t frame = 0; frame < n; ++frame) {
      ptn[frame] =
          Rect::FromLtwh(topleft.x, topleft.y, frame_size.x, frame_size.y);
      topleft.x += frame_size.x;
    }
  }

  void SetSquareSheet(PixelPoint topleft, std::size_t frame_count,
                      int frame_size, EnemyAnimationMode animation_mode) {
    SetSheet(topleft, frame_count, {.x = frame_size, .y = frame_size},
             animation_mode);
  }

  void SetDirectionalSheet(PixelPoint topleft, int frame_size) {
    SetSquareSheet(topleft, kEnemyAnimationFrameCapacity, frame_size,
                   EnemyAnimationMode::Directional);
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
    SetEnumFlag(flags, flag, static_cast<uint8_t>(enabled));
  }
  [[nodiscard]] bool IsHitBy(const PlayerAttack &attack) const;
  void UpdateAnimation(const EnemyAnimationSet &animations);

  EnemyActorState state = EnemyActorState::Active;

  WorldCoord x{}, y{}; // Display coordinates
  WorldCoord vx{}, vy{};

  WorldCoord v{};

  uint32_t hp{}; // Remaining HP (too large?)
  ItemKind item{};
  int count{}; // Multipurpose frame counter

  int score{}; // Score (time-based score variation?)

  EclScriptState script{};

  WorldCoord hitbox_half_width{};
  WorldCoord hitbox_half_height{};
  int animation_frame{};

  uint8_t d{}; // Direction angle 256
  int vd{};    // Angular velocity
  int amp{};   // Amplitude 256
  std::size_t animation{};
  std::size_t damage_animation{};
  int animation_speed{};
  int damage_flash{};

  EnemyActorFlags flags = EnemyActorFlags::None;
  int auto_fire_frame{};
  int auto_fire_interval{};

  int long_laser_count{};

  EclBulletState bullet_command{};
  EclLaserState laser_command{};
};
