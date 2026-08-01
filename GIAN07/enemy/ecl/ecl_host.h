///
/// EclHost - explicit game-services boundary used by the ECL VM
///

#pragma once

#include <cstdint>

#include "ecl.h"

#include "enemy/actor/enemy_actor.h"
#include "gfx/coords.h"

struct BulletManager;
struct GameSession;
class EnemyManager;
class Player;

namespace stage {
class StageSession;
}

class EclHost {
public:
  EclHost(EnemyManager &enemies, BulletManager &bullets, GameSession &session,
          Player &player, stage::StageSession &stage)
      : enemies_(enemies), bullets_(bullets), session_(session),
        player_(player), stage_(stage) {}

  [[nodiscard]] BulletManager &Bullets() const { return bullets_; }
  [[nodiscard]] GameSession &Session() const { return session_; }
  [[nodiscard]] Player &GetPlayer() const { return player_; }
  [[nodiscard]] stage::StageSession &Stage() const { return stage_; }
  [[nodiscard]] const EnemyAnimationSet &Animations() const;

  [[nodiscard]] EnemyActor *SpawnRegular(WorldPoint position,
                                         uint32_t script_id) const;
  void SpawnBoss(WorldPoint position, uint32_t script_id) const;
  void KillBosses() const;
  void ClearRegular() const;
  void ClearBossProjectiles() const;

  [[nodiscard]] uint16_t BossCount() const;
  [[nodiscard]] int BitCount(const EnemyActor &actor) const;
  void HandleBossAction(EnemyActor &actor, EclBossAction action) const;
  void SetBitAttack(EnemyActor &actor, uint32_t script_id) const;
  void ControlBitLaser(EnemyActor &actor, EclBitLaserCommand command) const;
  void ControlBits(EnemyActor &actor, EclBitCommand command,
                   int parameter) const;

private:
  EnemyManager &enemies_;
  BulletManager &bullets_;
  GameSession &session_;
  Player &player_;
  stage::StageSession &stage_;
};
