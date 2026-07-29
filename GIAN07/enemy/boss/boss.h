///
/// Boss actor state shared by the manager and formations
///

#pragma once

#include <cstddef>
#include <cstdint>

#include "enemy/actor/enemy_actor.h"

enum class BossMode : uint8_t {
  Normal,
  ButterflyWings,
  BirdWings,
  BombShield,
  BombSpirit,
};

struct BossActor : EnemyActor {
  void ResetForSpawn() {
    item = {};
    EnterMode(BossMode::Normal);
  }

  void EnterMode(BossMode next_mode) {
    mode = next_mode;
    mode_frame = 0;
  }

  void UpdateMode() {
    if (mode == BossMode::ButterflyWings &&
        mode_frame < kButterflyTransitionFrames) {
      ++mode_frame;
    }
  }

  [[nodiscard]] bool IsDefeated() const {
    return hp == 0 && state == EnemyActorState::Exploding;
  }

  [[nodiscard]] bool IsFinished() const {
    return state == EnemyActorState::PendingRemoval || IsDefeated();
  }

  BossMode mode = BossMode::Normal;
  uint32_t mode_frame = 0;

private:
  static constexpr uint32_t kButterflyTransitionFrames = 88;
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

inline constexpr size_t kBossCapacity = 4;
