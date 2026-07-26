///
/// EnemyActorRuntime - shared per-frame actor behavior
///

#pragma once

#include <cstdint>

#include "enemy_actor.h"

struct BulletManager;
struct GameManager;
class EnemySystem;
class Player;

enum class AutoFirePolicy : uint8_t {
  RequireHp,
  IgnoreHp,
};

class EnemyActorRuntime {
public:
  EnemyActorRuntime(EnemySystem &enemies, BulletManager &bullets,
                    GameManager &game, Player &player)
      : enemies_(&enemies), bullets_(&bullets), game_(&game), player_(&player) {
  }

  void BeginFrame(EnemyActor &actor, AutoFirePolicy auto_fire);
  void CheckPlayerCollision(const EnemyActor &actor) const;
  void FinishFrame(EnemyActor &actor, bool consider_homing);

private:
  EnemySystem *enemies_;
  BulletManager *bullets_;
  GameManager *game_;
  Player *player_;
};
