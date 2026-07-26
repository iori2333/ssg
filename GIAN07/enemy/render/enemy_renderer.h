///
/// EnemyRenderer - presentation of shared enemy actors
///

#pragma once

#include <array>

#include "core/object_pool.h"
#include "enemy/actor/enemy_actor.h"
#include "enemy/boss/bit_formation.h"
#include "enemy/boss/boss.h"

class Player;

class EnemyRenderer {
public:
  EnemyRenderer(const EnemyAnimationSet &animations, const Player &player)
      : animations_(&animations), player_(&player) {}

  void DrawRegular(const ObjectPool<EnemyActor, ENEMY_MAX> &actors) const;
  void DrawBosses(const ObjectPool<BossActor, BOSS_MAX> &bosses,
                  const std::array<BitFormation, BOSS_MAX> &formations) const;

private:
  void DrawActor(const EnemyActor &actor) const;
  void DrawExplosion(const EnemyActor &actor) const;
  void DrawBossLinks(const BitFormation &formation) const;
  bool DrawBossSpecialState(const BossActor &boss) const;

  const EnemyAnimationSet *animations_;
  const Player *player_;
};
