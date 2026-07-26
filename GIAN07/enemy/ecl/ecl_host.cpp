///
/// EclHost - ECL access to enemy and boss-owned game operations
///

#include "ecl_host.h"

#include "enemy/enemy_system.h"

const EnemyAnimationSet &EclHost::Animations() const {
  return enemies_->animations_;
}

EnemyActor *EclHost::SpawnRegular(WORLD_POINT position,
                                  uint32_t script_id) const {
  return enemies_->SpawnRegular(position, script_id);
}

void EclHost::SpawnBoss(WORLD_POINT position, uint32_t script_id) const {
  enemies_->bosses_.SpawnFromEcl(position, script_id);
}

void EclHost::KillBosses() const { enemies_->bosses_.KillActors(); }

void EclHost::ClearRegular() const { enemies_->ClearRegular(); }

void EclHost::ClearBossProjectiles() const {
  enemies_->bosses_.ClearProjectiles();
}

uint16_t EclHost::BossCount() const { return enemies_->bosses_.ActiveCount(); }

int EclHost::BitCount(const EnemyActor &actor) const {
  return enemies_->bosses_.BitCount(actor);
}

void EclHost::HandleBossAction(EnemyActor &actor, EclBossAction action) const {
  enemies_->bosses_.HandleAction(actor, action);
}

void EclHost::SetBitAttack(EnemyActor &actor, uint32_t script_id) const {
  enemies_->bosses_.SetBitAttack(actor, script_id);
}

void EclHost::ControlBitLaser(EnemyActor &actor,
                              EclBitLaserCommand command) const {
  enemies_->bosses_.ControlBitLaser(actor, command);
}

void EclHost::ControlBits(EnemyActor &actor, EclBitCommand command,
                          int parameter) const {
  enemies_->bosses_.ControlBits(actor, command, parameter);
}
