///
/// Enemy actor lifecycle, damage, and spawn control
///

#include <algorithm>
#include <cstddef>
#include <utility>

#include "enemy_system.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "util/cast.h"
#include "util/ut_math.h"

EnemySystem::EnemySystem(BulletManager &bullets, ItemManager &items,
                         GameManager &game, Player &player,
                         stage::StageSession &stage, EffectManager &effects)
    : renderer_(animations_, player), bullets_(&bullets), items_(&items),
      player_(&player), ecl_host_(*this, bullets, game, player, stage),
      ecl_(ecl_host_, effects), actor_runtime_(*this, bullets, game, player),
      bosses_(*this, bullets, items, player, stage, effects) {
  Reset();
}

bool EnemySystem::InstallStageAssets(EclProgram program,
                                     EnemyAnimationSet animations) {
  for (const auto &instruction : program.Instructions()) {
    uint8_t animation = 0;
    switch (instruction.Opcode()) {
    case EclOpcode::SetAnimation:
      animation = instruction.ArgumentsAs<EclAnimationArguments>().pattern;
      break;
    case EclOpcode::SetDamageAnimation:
      animation = instruction.ArgumentsAs<EclByteArguments>().value;
      break;
    default:
      continue;
    }
    if (animation >= animations.size() || animations[animation].n == 0) {
      return false;
    }
  }

  ecl_.Install(std::move(program));
  animations_ = std::move(animations);
  return true;
}

void EnemySystem::Reset() {
  bosses_.Reset();
  ResetRegular();
  ResetHomingTarget();
}

void EnemySystem::Update() {
  ResetHomingTarget();
  bosses_.Update();
  MoveRegular();
}

void EnemySystem::ResetHomingTarget() {
  homing_target_ = {};
  homing_distance_ = HOMING_DUMMY;
}

void EnemySystem::DrawBosses() {
  renderer_.DrawBosses(bosses_.actors_, bosses_.bits_);
}

void EnemySystem::SpawnBoss(PIXEL_POINT position, uint32_t script_id) {
  bosses_.Spawn(WORLD_POINT{position}, script_id);
}

void EnemySystem::KillBosses() { bosses_.KillActors(); }

void EnemySystem::SetBossTimeout(int32_t timeout_end) {
  bosses_.SetStageTimeout(timeout_end);
}

void EnemySystem::ConsiderHomingTarget(const EnemyActor &actor) {
  const int temp = player_->Y() - actor.y;

  if (temp < 0) {
    return;
  }

  if (temp < homing_distance_) {
    homing_distance_ = temp;
    homing_target_ = {.active = true, .x = actor.x, .y = actor.y};
  }
}

void EnemySystem::MoveRegular() {
  // ECL can spawn another regular enemy while this loop is running. Query the
  // pool size on every iteration so each actor keeps the legacy same-frame
  // update behavior.
  for (std::size_t i = 0; i < regular_enemies_.Size(); ++i) {
    auto *e = &regular_enemies_.Active(i);
    if (e->state == EnemyActorState::Active) {
      // Normal enemy processing
      actor_runtime_.BeginFrame(*e, AutoFirePolicy::RequireHp);
      actor_runtime_.CheckPlayerCollision(*e);

      // Out-of-bounds check
      if ((e->y < GY_MIN - e->hitbox_half_height) ||
          (e->y > GY_MAX + e->hitbox_half_height) ||
          (e->x < GX_MIN - e->hitbox_half_width) ||
          (e->x > GX_MAX + e->hitbox_half_width)) {
        if ((e->flag & EF_CLIP) == 0) {
          if (e->long_laser_count != 0U) {
            bullets_->ControlLongLaser(
                e, ECL_ALL_LONG_LASERS,
                LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
          }
          e->state = EnemyActorState::PendingRemoval;
        }
      }
    } else if (e->count >= (8 * ENEMY_BOMB_SPD) - 1) {
      e->state = EnemyActorState::PendingRemoval;
    }

    actor_runtime_.FinishFrame(*e, bosses_.ActiveCount() == 0);
  }

  CompactRegular();
}

void EnemySystem::DrawRegular() { renderer_.DrawRegular(regular_enemies_); }

// Clear small enemies
void EnemySystem::ClearRegular() {
  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (e->state == EnemyActorState::Exploding) {
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      e->state = EnemyActorState::Exploding;
      e->hp = 0;
      e->count = 0;
      if (e->long_laser_count != 0U) {
        bullets_->ControlLongLaser(
            e, ECL_ALL_LONG_LASERS,
            LongLaserUpdateInfo{
                LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
      }
      Snd_SEPlay(SfxId::Bomb, e->x);
    } else {
      // Erasing non-drawing type enemies differs from other cases:
      // do not play explosion animation/sound
      e->state = EnemyActorState::PendingRemoval;
      e->hp = 0;
      e->count = 0;
      if (e->long_laser_count != 0U) {
        bullets_->ControlLongLaser(
            e, ECL_ALL_LONG_LASERS,
            LongLaserUpdateInfo{
                LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
      }
      // Do not play explosion sound
    }
  }

  CompactRegular();
}

void EnemySystem::CompactRegular() {
  for (auto &actor : regular_enemies_) {
    if (actor.state == EnemyActorState::PendingRemoval) {
      RetireActor(actor);
    }
  }
  regular_enemies_.Compact([](const EnemyActor &actor) {
    return actor.state == EnemyActorState::PendingRemoval;
  });
}

void EnemySystem::RetireActor(EnemyActor &actor) {
  // Release every cross-frame observer before ObjectPool can reuse this slot.
  bosses_.OnActorRetired(actor);
  bullets_->ControlLongLaser(
      &actor, ECL_ALL_LONG_LASERS,
      LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
  actor.long_laser_count = 0;
}

void EnemySystem::ResetRegular() {
  for (auto &actor : regular_enemies_) {
    RetireActor(actor);
  }
  regular_enemies_.Init();
}

bool EnemySystem::ApplyDamage(EnemyActor &e, int damage) {
  e.damage_flash = e.count & 1;
  if (std::cmp_less_equal(e.hp, damage)) {
    Snd_SEPlay(SfxId::Bomb, e.x);
    if (e.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &e, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(static_cast<uint8_t>(e.hp)); // Power up
    e.hp = 0;
    e.count = 0;
    e.state = EnemyActorState::Exploding;
    player_->AddScore(e.score);
    if (e.item != 0U) {
      items_->Spawn(e.x, e.y, e.item);
    }
  } else {
    Snd_SEPlay(SfxId::Hit, e.x);
    player_->PowerUp(damage); // Power up here too
    e.hp -= damage;
  }
  return true;
}

bool EnemySystem::ApplyAttack(const EnemyAttack &attack) {
  bool hit = bosses_.ApplyAttack(attack);
  if (hit && attack.first_hit_only) {
    return true;
  }

  for (auto &actor : regular_enemies_) {
    if (actor.state != EnemyActorState::Active ||
        (actor.flag & EF_DAMAGE) == 0 || !EnemyAttackHits(actor, attack)) {
      continue;
    }
    hit = ApplyDamage(actor, attack.regular_damage);
    if (attack.first_hit_only) {
      return hit;
    }
  }
  return hit;
}

// Diagonal laser hit detection
// Directed beams use EnemyAttack::DirectedBeam through ApplyAttack().

void EnemySystem::InitializeActor(EnemyActor &actor, WORLD_POINT position,
                                  uint32_t script_id) {
  actor = {};

  if (!ecl_.Start(actor, script_id)) {
    actor.state = EnemyActorState::PendingRemoval;
    return;
  }

  actor.x = position.x;
  actor.y = position.y;

  actor.hp = 0xffffffff;
  actor.amp = 0;
  actor.animation = 0;
  actor.damage_animation = 0;
  actor.animation_speed = 0;
  actor.animation_frame = 0;
  actor.count = 0;
  actor.graze_score = 0;
  actor.d = 64;
  actor.flag = EF_DAMAGE | EF_DRAW | EF_HITSB;

  actor.damage_flash = 0;

  actor.auto_fire_frame = Cast::down<uint8_t>(rnd());
  actor.auto_fire_interval = 0;
  actor.hitbox_half_width = 0;
  actor.hitbox_half_height = 0;

  actor.item = ITEM_SCORE;

  actor.script.loop_counter = 0;
  actor.script.wait_counter = 0;
  actor.v = 64;
  actor.vd = 0;
  actor.vx = cosl(actor.d, actor.v);
  actor.vy = sinl(actor.d, actor.v);

  actor.long_laser_count = 0;

  actor.bullet_command.c = 0;
  actor.bullet_command.cmd = TC_WAY;
  actor.bullet_command.d = 64;
  actor.bullet_command.n = 1;
  actor.bullet_command.option = TE_NONE;
  actor.bullet_command.type = T_NORM;
  actor.bullet_command.v = 3;
  actor.bullet_command.x = 0;
  actor.bullet_command.y = 0;

  actor.bullet_command.dw = 16;
  actor.bullet_command.ns = 1;
  actor.bullet_command.rep = 0;
  actor.bullet_command.vd = 0;

  actor.laser_command.l2 = 0;
  actor.laser_command.x = 0;
  actor.laser_command.y = 0;
  actor.laser_command.notr = 0xff;
}

EnemyActor *EnemySystem::SpawnRegular(WORLD_POINT position,
                                      uint32_t script_id) {
  auto *actor = regular_enemies_.Alloc();
  if (actor == nullptr) {
    return nullptr;
  }
  InitializeActor(*actor, position, script_id);
  return actor;
}

void EnemySystem::SpawnFromScene(int16_t x, int16_t y, uint8_t script_id) {
  WORLD_POINT position;
  position.x = x == X_RNDV ? GX_RND() : PixelToWorld(x);
  position.y = y == Y_RNDV ? GY_RND() : PixelToWorld(y);
  SpawnRegular(position, script_id);
}

void EnemySystem::UpdateAnimation(EnemyActor &actor) {
  if (actor.animation >= animations_.size()) {
    actor.state = EnemyActorState::PendingRemoval;
    return;
  }
  const auto &animation = animations_[actor.animation];
  if (animation.n == 0) {
    actor.state = EnemyActorState::PendingRemoval;
    return;
  }

  switch (animation.mode) {
  case ANM_NORM:
    if (actor.animation_speed > 0 && actor.count % actor.animation_speed == 0) {
      actor.animation_frame = (actor.animation_frame + 1) % animation.n;
    } else if (actor.animation_speed < 0 &&
               actor.count % -actor.animation_speed == 0) {
      actor.animation_frame =
          (actor.animation_frame + animation.n - 1) % animation.n;
    }
    break;

  // Reverse direction is not allowed...
  case ANM_STOP:
    if (actor.animation_speed > 0 && actor.count % actor.animation_speed == 0) {
      if (actor.animation_frame < animation.n - 1) {
        actor.animation_frame++;
      }
    }
    break;

  default:
    break;
  }
}
