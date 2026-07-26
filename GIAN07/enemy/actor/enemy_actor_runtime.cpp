///
/// EnemyActorRuntime - shared per-frame actor behavior
///

#include "enemy_actor_runtime.h"

#include "bullet/bullet_manager.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "enemy/enemy_system.h"
#include "player/player.h"

void EnemyActorRuntime::BeginFrame(EnemyActor &actor,
                                   AutoFirePolicy auto_fire) {
  actor.damage_flash = 0;
  enemies_->ecl_.CheckInterrupts(actor);
  enemies_->ecl_.Execute(actor);

  const bool can_fire = auto_fire == AutoFirePolicy::IgnoreHp || actor.hp != 0;
  if (actor.auto_fire_interval == 0U || !can_fire) {
    return;
  }

  actor.auto_fire_frame =
      (actor.auto_fire_frame + 1) % actor.auto_fire_interval;
  if (actor.auto_fire_frame == 0) {
    auto spawn = MakeBulletSpawnInfo(actor.bullet_command, actor.x, actor.y,
                                     true, *game_);
    bullets_->SpawnBullet(spawn);
  }
}

void EnemyActorRuntime::CheckPlayerCollision(const EnemyActor &actor) const {
  if (HITCHK(actor.x, player_->X(), actor.hitbox_half_width) &&
      HITCHK(actor.y, player_->Y(), actor.hitbox_half_height) &&
      player_->IsInvincible() == 0 && (actor.flag & EF_HITSB) != 0) {
    player_->OnHit();
  }
}

void EnemyActorRuntime::FinishFrame(EnemyActor &actor, bool consider_homing) {
  if (consider_homing && (actor.flag & EF_DAMAGE) != 0) {
    enemies_->ConsiderHomingTarget(actor);
  }
  enemies_->UpdateAnimation(actor);
  ++actor.count;
}
