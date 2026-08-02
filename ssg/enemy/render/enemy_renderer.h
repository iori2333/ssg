///
/// EnemyRenderer - presentation of shared enemy actors
///

#pragma once

#include <array>

#include "enemy/actor/enemy_actor.h"
#include "enemy/boss/bit_formation.h"
#include "enemy/boss/boss.h"
#include "util/object_pool.h"

class Player;

class EnemyRenderer {
public:
  EnemyRenderer(const EnemyAnimationSet &animations, const Player &player)
      : animations_(animations), player_(player) {}

  void
  DrawRegular(const util::ObjectPool<EnemyActor, kEnemyCapacity> &actors) const;
  void
  DrawBosses(const util::ObjectPool<BossActor, kBossCapacity> &bosses,
             const std::array<BitFormation, kBossCapacity> &formations) const;

private:
  void DrawActor(const EnemyActor &actor) const;
  static void DrawExplosion(const EnemyActor &actor);
  static void DrawBossLinks(const BitFormation &formation);
  [[nodiscard]] bool DrawBossSpecialState(const BossActor &boss) const;

  const EnemyAnimationSet &animations_;
  const Player &player_;
};
