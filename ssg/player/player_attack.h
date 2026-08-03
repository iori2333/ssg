///
/// Attacks that can be applied to regular enemies and bosses
///

#pragma once

#include <cstdint>

#include "gfx/core/coords.h"

enum class PlayerAttackShape : uint8_t {
  Point,
  VerticalBeam,
  DirectedBeam,
  AllEnemies,
};

struct PlayerAttack {
  PlayerAttackShape shape = PlayerAttackShape::Point;
  WorldPoint origin;
  uint8_t direction = 0;
  int regular_damage = 0;
  int boss_damage = 0;
  bool first_hit_only = false;

  [[nodiscard]] static PlayerAttack Point(WorldPoint origin, int damage) {
    return {.shape = PlayerAttackShape::Point,
            .origin = origin,
            .regular_damage = damage,
            .boss_damage = damage,
            .first_hit_only = true};
  }

  [[nodiscard]] static PlayerAttack VerticalBeam(WorldPoint origin,
                                                 int damage) {
    return {.shape = PlayerAttackShape::VerticalBeam,
            .origin = origin,
            .regular_damage = damage,
            .boss_damage = damage};
  }

  [[nodiscard]] static PlayerAttack DirectedBeam(WorldPoint origin,
                                                 uint8_t direction) {
    return {.shape = PlayerAttackShape::DirectedBeam,
            .origin = origin,
            .direction = direction,
            .regular_damage = 8,
            .boss_damage = 2};
  }

  [[nodiscard]] static PlayerAttack AllEnemies(int damage) {
    return {.shape = PlayerAttackShape::AllEnemies,
            .regular_damage = damage,
            .boss_damage = damage};
  }
};
