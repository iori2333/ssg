///
/// Boss actor state shared by the manager and formations
///

#pragma once

#include <cstddef>
#include <cstdint>

#include "enemy/actor/enemy_actor.h"

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

struct BossHudModel {
  uint64_t encounter_revision = 0;
  uint64_t phase_revision = 0;
  bool active = false;
  uint32_t max_hp = 0;
  uint32_t phase_hp = 0;
  uint32_t current_hp = 0;
  int32_t phase_threshold_hp = -1;
  int32_t timer_max = -1;
  int32_t timer_now = 0;
  int32_t stage_timeout_end = -1;
};

inline constexpr size_t BOSS_MAX = 4;
