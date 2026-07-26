///
/// Enemy actor lifecycle, damage, and spawn control
///

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <utility>

#include "enemy_manager.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/game_manager.h"
#include "core/gian.h"
#include "core/level.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "util/cast.h"
#include "util/ut_math.h"

EnemyManager::EnemyManager(BulletManager &bullets, ItemManager &items,
                           GameManager &game, Player &player,
                           stage::StageSession &stage, EffectManager &effects)
    : renderer_(animations_, player), bullets_(&bullets), game_(&game),
      items_(&items), player_(&player), stage_(&stage), effects_(&effects),
      ecl_host_(*this, bullets, game, player, stage), ecl_(ecl_host_, effects),
      snakes_(*this, bullets), bits_{BitFormation(*this, bullets, player),
                                     BitFormation(*this, bullets, player),
                                     BitFormation(*this, bullets, player),
                                     BitFormation(*this, bullets, player)} {
  Reset();
}

bool EnemyManager::InstallStageAssets(EclProgram program,
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

void EnemyManager::Reset() {
  ResetBosses();
  ResetRegular();
  ResetHomingTarget();
}

void EnemyManager::Update() {
  ResetHomingTarget();
  UpdateBosses();
  UpdateRegular();
}

void EnemyManager::ResetHomingTarget() {
  homing_target_ = {};
  homing_distance_ = HOMING_DUMMY;
}

void EnemyManager::DrawBosses() { renderer_.DrawBosses(bosses_, bits_); }

void EnemyManager::SpawnBoss(PIXEL_POINT position, uint32_t script_id) {
  SpawnBossActor(WORLD_POINT{position}, script_id, true);
}

void EnemyManager::ConsiderHomingTarget(const EnemyActor &actor) {
  const int temp = player_->Y() - actor.y;

  if (temp < 0) {
    return;
  }

  if (temp < homing_distance_) {
    homing_distance_ = temp;
    homing_target_ = {.active = true, .x = actor.x, .y = actor.y};
  }
}

void EnemyManager::BeginActorFrame(EnemyActor &actor,
                                   bool allow_fire_with_zero_hp) {
  actor.damage_flash = 0;
  ecl_.CheckInterrupts(actor);
  ecl_.Execute(actor);

  if (actor.auto_fire_interval == 0U ||
      (!allow_fire_with_zero_hp && actor.hp == 0)) {
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

void EnemyManager::CheckPlayerCollision(const EnemyActor &actor) const {
  const int dx =
      std::max(std::abs(player_->X() - actor.x) - actor.hitbox_half_width, 0);
  const int dy =
      std::max(std::abs(player_->Y() - actor.y) - actor.hitbox_half_height, 0);
  const auto radius = static_cast<int64_t>(player_->HitRadius());
  if (static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy <=
          radius * radius &&
      !player_->IsInvincible() && (actor.flag & EF_HITSB) != 0) {
    player_->OnHit();
  }
}

void EnemyManager::FinishActorFrame(EnemyActor &actor, bool consider_homing) {
  if (consider_homing && (actor.flag & EF_DAMAGE) != 0) {
    ConsiderHomingTarget(actor);
  }
  actor.UpdateAnimation(animations_);
  ++actor.count;
}

void EnemyManager::OnActorRetired(const EnemyActor &actor) {
  snakes_.OnActorRetired(actor);
  for (auto &bits : bits_) {
    bits.OnActorRetired(actor);
  }
}

void EnemyManager::UpdateRegular() {
  // ECL can spawn another regular enemy while this loop is running. Query the
  // pool size on every iteration so each actor keeps the legacy same-frame
  // update behavior.
  for (std::size_t i = 0; i < regular_enemies_.Size(); ++i) {
    auto *e = &regular_enemies_.Active(i);
    if (e->state == EnemyActorState::Active) {
      // Normal enemy processing
      BeginActorFrame(*e, false);
      CheckPlayerCollision(*e);

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

    FinishActorFrame(*e, bosses_.IsEmpty());
  }

  CompactRegular();
}

void EnemyManager::DrawRegular() { renderer_.DrawRegular(regular_enemies_); }

// Clear small enemies
void EnemyManager::ClearRegular() {
  for (auto &actor : regular_enemies_) {
    auto *e = &actor;
    if (e->state == EnemyActorState::Exploding) {
      continue;
    }

    if ((e->flag & EF_DRAW) != 0) {
      e->BeginExplosion();
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

void EnemyManager::CompactRegular() {
  for (auto &actor : regular_enemies_) {
    if (actor.state == EnemyActorState::PendingRemoval) {
      RetireActor(actor);
    }
  }
  regular_enemies_.Compact([](const EnemyActor &actor) {
    return actor.state == EnemyActorState::PendingRemoval;
  });
}

void EnemyManager::RetireActor(EnemyActor &actor) {
  // Release every cross-frame observer before ObjectPool can reuse this slot.
  OnActorRetired(actor);
  bullets_->ControlLongLaser(
      &actor, ECL_ALL_LONG_LASERS,
      LongLaserUpdateInfo{LongLaserUpdateInfo::Command::ForceClose});
  actor.long_laser_count = 0;
}

void EnemyManager::ResetRegular() {
  for (auto &actor : regular_enemies_) {
    RetireActor(actor);
  }
  regular_enemies_.Init();
}

void EnemyManager::ApplyRegularDamage(EnemyActor &actor, int damage) {
  actor.damage_flash = actor.count & 1;
  if (std::cmp_less_equal(actor.hp, damage)) {
    Snd_SEPlay(SfxId::Bomb, actor.x);
    if (actor.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &actor, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(static_cast<uint8_t>(actor.hp)); // Power up
    actor.BeginExplosion();
    player_->AddScore(actor.score);
    if (actor.item != 0U) {
      items_->Spawn(actor.x, actor.y, actor.item);
    }
  } else {
    Snd_SEPlay(SfxId::Hit, actor.x);
    player_->PowerUp(damage); // Power up here too
    actor.hp -= damage;
  }
}

bool EnemyManager::ApplyPlayerAttack(const PlayerAttack &attack) {
  bool hit = ApplyPlayerAttackToBosses(attack);
  if (hit && attack.first_hit_only) {
    return true;
  }

  for (auto &actor : regular_enemies_) {
    if (actor.state != EnemyActorState::Active ||
        (actor.flag & EF_DAMAGE) == 0 || !actor.IsHitBy(attack)) {
      continue;
    }
    ApplyRegularDamage(actor, attack.regular_damage);
    hit = true;
    if (attack.first_hit_only) {
      return hit;
    }
  }
  return hit;
}

// Diagonal laser hit detection
// Directed beams use PlayerAttack::DirectedBeam through ApplyPlayerAttack().

void EnemyManager::InitializeActor(EnemyActor &actor, WORLD_POINT position,
                                   uint32_t script_id) {
  actor = {};

  if (!ecl_.Start(actor, script_id)) {
    actor.state = EnemyActorState::PendingRemoval;
    return;
  }

  actor.x = position.x;
  actor.y = position.y;

  actor.hp = std::numeric_limits<uint32_t>::max();
  actor.d = 64;
  actor.flag = EF_DAMAGE | EF_DRAW | EF_HITSB;
  actor.auto_fire_frame = Cast::down<uint8_t>(rnd());
  actor.item = ITEM_SCORE;
  actor.v = 64;
  actor.vx = cosl(actor.d, actor.v);
  actor.vy = sinl(actor.d, actor.v);

  actor.bullet_command = {.d = 64,
                          .dw = 16,
                          .n = 1,
                          .ns = 1,
                          .v = 3,
                          .cmd = TC_WAY,
                          .type = T_NORM,
                          .option = TE_NONE};
  actor.laser_command = {.notr = 0xff};
}

EnemyActor *EnemyManager::SpawnRegular(WORLD_POINT position,
                                       uint32_t script_id) {
  auto *actor = regular_enemies_.Alloc();
  if (actor == nullptr) {
    return nullptr;
  }
  InitializeActor(*actor, position, script_id);
  return actor;
}

void EnemyManager::SpawnFromScene(int16_t x, int16_t y, uint8_t script_id) {
  WORLD_POINT position;
  position.x = x == X_RNDV ? GX_RND() : PixelToWorld(x);
  position.y = y == Y_RNDV ? GY_RND() : PixelToWorld(y);
  SpawnRegular(position, script_id);
}
