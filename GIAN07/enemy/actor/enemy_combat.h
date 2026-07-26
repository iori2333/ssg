///
/// Enemy combat queries shared by regular enemies and bosses
///

#pragma once

#include <cstdint>

#include "enemy_actor.h"

#include "gfx/coords.h"

enum class EnemyAttackShape : uint8_t {
  Point,
  VerticalBeam,
  DirectedBeam,
  All,
};

struct EnemyAttack {
  EnemyAttackShape shape = EnemyAttackShape::Point;
  WORLD_POINT origin{};
  uint8_t direction = 0;
  int regular_damage = 0;
  int boss_damage = 0;
  bool first_hit_only = false;

  [[nodiscard]] static EnemyAttack Point(WORLD_POINT origin, int damage) {
    return {.shape = EnemyAttackShape::Point,
            .origin = origin,
            .regular_damage = damage,
            .boss_damage = damage,
            .first_hit_only = true};
  }

  [[nodiscard]] static EnemyAttack VerticalBeam(WORLD_POINT origin,
                                                int damage) {
    return {.shape = EnemyAttackShape::VerticalBeam,
            .origin = origin,
            .regular_damage = damage,
            .boss_damage = damage};
  }

  [[nodiscard]] static EnemyAttack DirectedBeam(WORLD_POINT origin,
                                                uint8_t direction) {
    return {.shape = EnemyAttackShape::DirectedBeam,
            .origin = origin,
            .direction = direction,
            .regular_damage = 8,
            .boss_damage = 2};
  }

  [[nodiscard]] static EnemyAttack All(int damage) {
    return {.shape = EnemyAttackShape::All,
            .regular_damage = damage,
            .boss_damage = damage};
  }
};

[[nodiscard]] bool EnemyAttackHits(const EnemyActor &actor,
                                   const EnemyAttack &attack);
