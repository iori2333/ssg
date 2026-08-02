///
/// Enemy actor lifecycle, damage, and spawn control
///

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>

#include "enemy_manager.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "bullet/bullet.h"
#include "bullet/bullet_common.h"
#include "bullet/bullet_manager.h"
#include "bullet/laser/long.h"
#include "enemy/actor/enemy_actor.h"
#include "enemy/ecl/ecl.h"
#include "enemy/ecl/ecl_program.h"
#include "gameplay/game_session.h"
#include "gameplay/playfield.h"
#include "gfx/coords.h"
#include "item/item_system.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "util/math_utils.h"

namespace {

constexpr int kRandomCoordinate = -30000;

int RandomWorldX() {
  return PixelToWorld(playfield::kLeft +
                      math::RandomInt() %
                          (playfield::kRight - playfield::kLeft));
}

int RandomWorldY() {
  return PixelToWorld(playfield::kTop +
                      math::RandomInt() %
                          (playfield::kBottom - playfield::kTop));
}

} // namespace

EnemyManager::EnemyManager(BulletManager &bullets, ItemSystem &items,
                           GameSession &session, Player &player,
                           stage::StageSession &stage, EffectManager &effects,
                           audio::AudioSystem &audio)
    : renderer_(animations_, player), bullets_(bullets), session_(session),
      items_(items), player_(player), stage_(stage), effects_(effects),
      audio_(audio), ecl_host_(*this, bullets, session, player, stage),
      ecl_(ecl_host_, effects, audio), snakes_(*this, bullets),
      bits_{BitFormation(*this, bullets, player, audio),
            BitFormation(*this, bullets, player, audio),
            BitFormation(*this, bullets, player, audio),
            BitFormation(*this, bullets, player, audio)} {
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
  animations_ = animations;
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
  homing_distance_ = kNoHomingDistance;
}

void EnemyManager::DrawBosses() { renderer_.DrawBosses(bosses_, bits_); }

void EnemyManager::SpawnBoss(PixelPoint position, uint32_t script_id) {
  SpawnBossActor(WorldPoint{position}, script_id, true);
}

void EnemyManager::ConsiderHomingTarget(const EnemyActor &actor) {
  const int temp = player_.Y() - actor.y;

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
                                     true, session_);
    bullets_.SpawnBullet(spawn);
  }
}

void EnemyManager::CheckPlayerCollision(const EnemyActor &actor) const {
  const int dx =
      std::max(std::abs(player_.X() - actor.x) - actor.hitbox_half_width, 0);
  const int dy =
      std::max(std::abs(player_.Y() - actor.y) - actor.hitbox_half_height, 0);
  const auto radius = static_cast<int64_t>(player_.HitRadius());
  if (static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy <=
          radius * radius &&
      !player_.IsInvincible() &&
      actor.HasFlag(EnemyActorFlags::CollidesWithPlayer)) {
    player_.OnHit();
  }
}

void EnemyManager::FinishActorFrame(EnemyActor &actor, bool consider_homing) {
  if (consider_homing && actor.HasFlag(EnemyActorFlags::Damageable)) {
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
      if ((e->y < playfield::kWorldTop - e->hitbox_half_height) ||
          (e->y > playfield::kWorldBottom + e->hitbox_half_height) ||
          (e->x < playfield::kWorldLeft - e->hitbox_half_width) ||
          (e->x > playfield::kWorldRight + e->hitbox_half_width)) {
        if (!e->HasFlag(EnemyActorFlags::KeepOutsidePlayfield)) {
          if (e->long_laser_count != 0U) {
            bullets_.ControlLongLaser(
                e, kEclAllLongLasers,
                LongLaserUpdateInfo{
                    .command = LongLaserUpdateInfo::Command::ForceClose});
          }
          e->state = EnemyActorState::PendingRemoval;
        }
      }
    } else if (e->count >= (8 * kEnemyExplosionSpeed) - 1) {
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

    if (e->HasFlag(EnemyActorFlags::Draw)) {
      e->BeginExplosion();
      if (e->long_laser_count != 0U) {
        bullets_.ControlLongLaser(
            e, kEclAllLongLasers,
            LongLaserUpdateInfo{
                .command =
                    LongLaserUpdateInfo::Command::ForceClose}); // Force close
                                                                // laser
      }
      audio_.PlaySfx(SfxId::Bomb, e->x);
    } else {
      // Erasing non-drawing type enemies differs from other cases:
      // do not play explosion animation/sound
      e->state = EnemyActorState::PendingRemoval;
      e->hp = 0;
      e->count = 0;
      if (e->long_laser_count != 0U) {
        bullets_.ControlLongLaser(
            e, kEclAllLongLasers,
            LongLaserUpdateInfo{
                .command =
                    LongLaserUpdateInfo::Command::ForceClose}); // Force close
                                                                // laser
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
  bullets_.ControlLongLaser(
      &actor, kEclAllLongLasers,
      LongLaserUpdateInfo{.command = LongLaserUpdateInfo::Command::ForceClose});
  actor.long_laser_count = 0;
}

void EnemyManager::ResetRegular() {
  for (auto &actor : regular_enemies_) {
    RetireActor(actor);
  }
  regular_enemies_.Reset();
}

void EnemyManager::ApplyRegularDamage(EnemyActor &actor, int damage) {
  actor.damage_flash = actor.count & 1;
  if (std::cmp_less_equal(actor.hp, damage)) {
    audio_.PlaySfx(SfxId::Bomb, actor.x);
    if (actor.long_laser_count != 0U) {
      bullets_.ControlLongLaser(
          &actor, kEclAllLongLasers,
          LongLaserUpdateInfo{.command =
                                  LongLaserUpdateInfo::Command::ForceClose});
    }
    player_.PowerUp(static_cast<uint8_t>(actor.hp));
    actor.BeginExplosion();
    player_.AddScore(actor.score);
    if (actor.item != ItemKind::None) {
      items_.Spawn(actor.x, actor.y, actor.item);
    }
  } else {
    audio_.PlaySfx(SfxId::Hit, actor.x);
    player_.PowerUp(damage);
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
        !actor.HasFlag(EnemyActorFlags::Damageable) || !actor.IsHitBy(attack)) {
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

void EnemyManager::InitializeActor(EnemyActor &actor, WorldPoint position,
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
  actor.flags = EnemyActorFlags::Damageable | EnemyActorFlags::Draw |
                EnemyActorFlags::CollidesWithPlayer;
  actor.auto_fire_frame = static_cast<uint8_t>(math::RandomInt());
  actor.item = ItemKind::Score;
  actor.v = 64;
  const auto velocity =
      math::RoundedPolarVector(math::AngleFromLegacy(actor.d), actor.v);
  actor.vx = velocity.x;
  actor.vy = velocity.y;

  actor.bullet_command = {.d = 64,
                          .dw = 16,
                          .n = 1,
                          .ns = 1,
                          .v = 3,
                          .cmd = std::to_underlying(BulletPattern::Spread),
                          .type = std::to_underlying(BulletMotion::Normal),
                          .option = 0};
  actor.laser_command = {};
}

EnemyActor *EnemyManager::SpawnRegular(WorldPoint position,
                                       uint32_t script_id) {
  auto *actor = regular_enemies_.Alloc();
  if (actor == nullptr) {
    return nullptr;
  }
  InitializeActor(*actor, position, script_id);
  return actor;
}

void EnemyManager::SpawnFromScene(int16_t x, int16_t y, uint8_t script_id) {
  WorldPoint position;
  position.x = x == kRandomCoordinate ? RandomWorldX() : PixelToWorld(x);
  position.y = y == kRandomCoordinate ? RandomWorldY() : PixelToWorld(y);
  SpawnRegular(position, script_id);
}
