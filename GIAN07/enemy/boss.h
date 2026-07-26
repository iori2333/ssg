///
/// Boss actor state shared by the manager and formations
///

#pragma once

#include <cstddef>
#include <cstdint>

#include "enemy.h"

enum class BossState : uint8_t {
  Normal,
  ButterflyWings,
  BirdWings,
  BombShield,
  BombSpirit,
};

struct BossData {
  EnemyActor actor;
  uint32_t state_frame = 0; // State counter (zeroed on transition)
  BossState state = BossState::Normal;
};
inline constexpr size_t BOSS_MAX = 4;
