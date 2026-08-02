///
/// EclHost - ECL access to enemy and boss-owned game operations
///

#include "ecl_host.h"

#include "enemy/actor/enemy_actor.h"
#include "enemy/ecl/ecl.h"
#include "enemy/enemy_manager.h"
#include "gfx/coords.h"
#include <cstdint>

const EnemyAnimationSet &EclHost::Animations() const {
  return enemies_.animations_;
}

EnemyActor *EclHost::SpawnRegular(WorldPoint position,
                                  uint32_t script_id) const {
  return enemies_.SpawnRegular(position, script_id);
}

void EclHost::SpawnBoss(WorldPoint position, uint32_t script_id) const {
  enemies_.SpawnBossFromEcl(position, script_id);
}

void EclHost::KillBosses() const { enemies_.KillBosses(); }

void EclHost::ClearRegular() const { enemies_.ClearRegular(); }

void EclHost::ClearBossProjectiles() const { enemies_.ClearBossProjectiles(); }

uint16_t EclHost::BossCount() const { return enemies_.BossCount(); }

int EclHost::BitCount(const EnemyActor &actor) const {
  return enemies_.BitCount(actor);
}

void EclHost::HandleBossAction(EnemyActor &actor, EclBossAction action) const {
  enemies_.HandleBossAction(actor, action);
}

void EclHost::SetBitAttack(EnemyActor &actor, uint32_t script_id) const {
  enemies_.SetBitAttack(actor, script_id);
}

void EclHost::ControlBitLaser(EnemyActor &actor,
                              EclBitLaserCommand command) const {
  enemies_.ControlBitLaser(actor, command);
}

void EclHost::ControlBits(EnemyActor &actor, EclBitCommand command,
                          int parameter) const {
  enemies_.ControlBits(actor, command, parameter);
}
