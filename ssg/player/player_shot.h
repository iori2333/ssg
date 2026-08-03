///
/// PlayerShot - Maid shot processing
///

#pragma once

#include <cstddef>
#include <cstdint>

#include "gfx/core/coords.h"

struct EnemyHomingTarget;

inline constexpr std::size_t kPlayerShotCapacity = 200;

enum class PlayerShotKind : uint8_t {
  WideMain,
  WideSub,
  HomingMain,
  HomingSub,
  LaserSub,
  HomingBomb,
  HomingBombBlast,
  WideFocusMain,
  WideFocusSub,
  HomingFocusMain,
  HomingFocusSub,
  Count,
};

enum class PlayerShotMotion : uint8_t {
  Straight,
  Homing,
  Stationary,
};

struct PlayerShot {
  WorldCoord x_{}, y_{};
  WorldCoord velocity_x_{}, velocity_y_{};
  WorldCoord speed_{};
  WorldCoord acceleration_{};
  float direction_{};
  int turn_rate_{};
  PlayerShotKind kind_{};
  PlayerShotMotion motion_{};
  int age_{};
  bool pending_removal_{};

  [[nodiscard]] bool Move(const EnemyHomingTarget &target);
};

struct PlayerShotSpawnInfo {
  WorldCoord x{}, y{};
  uint8_t direction{};
  uint8_t direction_step{};
  int count{};
  WorldCoord speed{};
  WorldCoord acceleration{};
  PlayerShotKind kind{};
  PlayerShotMotion motion = PlayerShotMotion::Straight;
  int turn_rate = 0;
};
