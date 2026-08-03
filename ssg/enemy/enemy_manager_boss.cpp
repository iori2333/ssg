///
/// Boss actor lifecycle and subsystem orchestration
///

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <utility>

#include "actor/enemy_actor.h"
#include "boss/boss.h"
#include "ecl/ecl.h"
#include "enemy_manager.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "bullet/bullet_manager.h"
#include "bullet/laser/long.h"
#include "effect/effect_manager.h"
#include "effect/effect_types.h"
#include "gfx/core/coords.h"
#include "item/item_system.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "stage/stage_background.h"
#include "stage/stage_session.h"

void EnemyManager::ResetBosses() {
  snakes_.Reset();
  for (auto &bits : bits_) {
    bits.Reset();
  }

  for (auto &boss : bosses_) {
    RetireActor(boss);
  }
  bosses_.Reset();

  const auto next_revision = boss_hud_.encounter_revision + 1;
  boss_hud_ = {.encounter_revision = next_revision};
}

void EnemyManager::SpawnBossFromEcl(WorldPoint position, uint32_t script_id) {
  SpawnBossActor(position, script_id, false);
}

void EnemyManager::SpawnBossActor(WorldPoint position, uint32_t script_id,
                                  bool starts_encounter) {
  auto *boss = bosses_.Alloc();
  if (boss == nullptr) {
    return;
  }

  InitializeActor(*boss, position, script_id);
  boss->ResetForSpawn();
  ecl_.Execute(*boss);

  const auto total_hp = TotalBossHp();
  if (starts_encounter) {
    ++boss_hud_.encounter_revision;
    boss_hud_.phase_revision = 1;
    boss_hud_.active = true;
    boss_hud_.max_hp = total_hp;
    boss_hud_.phase_hp = total_hp;
    boss_hud_.stage_timeout_end = -1;
  } else {
    ++boss_hud_.phase_revision;
    boss_hud_.phase_hp = total_hp;
  }
}
void EnemyManager::UpdateBosses() {
  uint32_t hp_sum = 0;

  // A boss ECL block can spawn another boss. Re-read the pool size so the new
  // actor retains the established same-frame update behavior.
  for (std::size_t index = 0; index < bosses_.Size(); ++index) {
    auto &boss = bosses_.Active(index);
    BeginActorFrame(boss, true);
    if (boss.state == EnemyActorState::Active) {
      boss.UpdateMode();
      CheckPlayerCollision(boss);
      hp_sum += boss.hp;
    }

    FinishActorFrame(boss, true);
  }

  int phase_threshold_hp = -1;
  int timer_max = -1;
  int timer_now = 0;
  for (const auto &boss : bosses_) {
    const auto &hp_interrupt =
        boss.script.interrupts[static_cast<size_t>(EclInterrupt::Hp)];
    const auto &timer_interrupt =
        boss.script.interrupts[static_cast<size_t>(EclInterrupt::Timer)];
    if (phase_threshold_hp < 0 && hp_interrupt.target) {
      phase_threshold_hp = hp_interrupt.threshold;
    }
    if (timer_max < 0 && timer_interrupt.target) {
      timer_max = timer_interrupt.threshold;
      timer_now = boss.script.interrupt_timer;
    }
  }

  RemoveFinishedBosses();
  snakes_.Update();
  for (auto &bits : bits_) {
    bits.Update();
  }
  boss_hud_.active = !bosses_.IsEmpty();
  boss_hud_.current_hp = hp_sum;
  boss_hud_.phase_threshold_hp = phase_threshold_hp;
  boss_hud_.timer_max = timer_max;
  boss_hud_.timer_now = timer_now;
}

void EnemyManager::ClearBossProjectiles() {
  for (auto &bits : bits_) {
    bits.Destroy();
  }
}

void EnemyManager::KillBosses() {
  for (auto &bits : bits_) {
    bits.Destroy();
  }

  for (auto &boss : bosses_) {
    snakes_.Remove(boss);
    effects_.SpawnFragment(boss.x, boss.y, FragmentKind::ExpandingCircle);
    effects_.SpawnBombExplosion(boss.x, boss.y);
    audio_.PlaySfx(SfxId::Bossbomb, boss.x);
    if (boss.long_laser_count != 0U) {
      bullets_.ControlLongLaser(
          &boss, kEclAllLongLasers,
          LongLaserUpdateInfo{.command =
                                  LongLaserUpdateInfo::Command::ForceClose});
    }
    boss.BeginExplosion();
    RetireActor(boss);
  }

  bosses_.Reset();
}

void EnemyManager::ApplyBossDamage(BossActor &boss, int damage) {
  EnemyActor &actor = boss;
  actor.damage_flash = actor.count & 1;
  if (std::cmp_less_equal(actor.hp, damage)) {
    snakes_.Remove(boss);
    if (auto *bits = FindBits(actor)) {
      bits->Destroy();
    }
    ClearRegular();
    effects_.SpawnFragment(actor.x, actor.y, FragmentKind::ExpandingCircle);
    effects_.SpawnBombExplosion(actor.x, actor.y);
    stage_.Command(stage::BackgroundCommand::Quake, effects_);
    audio_.PlaySfx(SfxId::Bossbomb, actor.x);
    if (actor.long_laser_count != 0U) {
      bullets_.ControlLongLaser(
          &actor, kEclAllLongLasers,
          LongLaserUpdateInfo{.command =
                                  LongLaserUpdateInfo::Command::ForceClose});
    }
    player_.PowerUp(static_cast<int>(actor.hp));
    actor.BeginExplosion();

    // If it was the last one //
    if (std::ranges::all_of(
            bosses_, [](const auto &boss) { return boss.IsDefeated(); })) {
      const auto temp = bullets_.ConvertBulletsToScore();
      effects_.SpawnString(180, 60, std::format("  Bonus    {:7}Pts", temp));
      player_.AddScore(temp);
    }

    if (actor.item != ItemKind::None) {
      items_.Spawn(actor.x, actor.y, actor.item);
    }
    player_.AddScore(actor.score);
    bullets_.Clear();
  } else {
    audio_.PlaySfx(SfxId::Hit, actor.x);
    player_.PowerUp(damage);
    actor.hp -= damage;
  }
}

bool EnemyManager::ApplyPlayerAttackToBosses(const PlayerAttack &attack) {
  bool hit = false;
  for (auto &boss : bosses_) {
    if (boss.mode != BossMode::Normal &&
        static_cast<unsigned int>(player_.IsBombActive()) != 0U) {
      continue;
    }

    if (boss.state != EnemyActorState::Active ||
        !boss.HasFlag(EnemyActorFlags::Damageable) || !boss.IsHitBy(attack)) {
      continue;
    }
    const int damage = attack.boss_damage - (BitCount(boss) >> 1);
    if (damage <= 0) {
      continue;
    }
    ApplyBossDamage(boss, damage);
    hit = true;
    if (attack.first_hit_only) {
      break;
    }
  }
  RemoveFinishedBosses();
  return hit;
}

void EnemyManager::RemoveFinishedBosses() {
  for (auto &boss : bosses_) {
    if (!boss.IsFinished()) {
      continue;
    }
    snakes_.Remove(boss);
    if (auto *bits = FindBits(boss)) {
      bits->Destroy();
    }
    RetireActor(boss);
  }
  bosses_.Compact([](const auto &boss) { return boss.IsFinished(); });
}

uint32_t EnemyManager::TotalBossHp() const {
  uint32_t total = 0;
  for (const auto &boss : bosses_) {
    total += boss.hp;
  }
  return total;
}

void EnemyManager::HandleBossAction(EnemyActor &actor, EclBossAction action) {
  auto b = std::ranges::find_if(bosses_, [&actor](auto &boss) {
    return static_cast<EnemyActor *>(&boss) == &actor;
  });
  if (b == bosses_.end()) {
    return;
  }

  switch (action) {
  case EclBossAction::EnableSnake:
    snakes_.Spawn(*b, 11);
    break;

  case EclBossAction::ButterflyWings: // Also draw butterfly wings
    b->EnterMode(BossMode::ButterflyWings);
    break;

  case EclBossAction::BirdWings: // Also draw bird wings
    b->EnterMode(BossMode::BirdWings);
    break;

  case EclBossAction::EnableFiveBits:
    if (auto *bits = AcquireBits(*b)) {
      bits->Spawn(*b, 5, 3);
    }
    break;

  case EclBossAction::EnableSixBits:
    if (auto *bits = AcquireBits(*b)) {
      bits->Spawn(*b, 6, 3);
    }
    break;

  case EclBossAction::BombShield:
    b->EnterMode(BossMode::BombShield);
    break;

  case EclBossAction::BombSpirit:
    b->EnterMode(BossMode::BombSpirit);
    break;
  }
}

void EnemyManager::SetBitAttack(EnemyActor &actor, uint32_t script_id) {
  if (auto *bits = FindBits(actor)) {
    bits->SelectAttack(script_id);
  }
}

void EnemyManager::ControlBitLaser(EnemyActor &actor,
                                   EclBitLaserCommand command) {
  if (auto *bits = FindBits(actor)) {
    bits->LaserCommand(command);
  }
}

void EnemyManager::ControlBits(EnemyActor &actor, EclBitCommand command,
                               int param) {
  if (auto *bits = FindBits(actor)) {
    bits->Command(command, param);
  }
}

void EnemyManager::SetBossTimeout(int timeout_end) {
  boss_hud_.stage_timeout_end = timeout_end;
}

int EnemyManager::BitCount(const EnemyActor &actor) const {
  const auto *bits = FindBits(actor);
  return bits != nullptr ? bits->Count() : 0;
}

BitFormation *EnemyManager::AcquireBits(BossActor &boss) {
  if (auto *bits = FindBits(boss)) {
    return bits;
  }
  auto available = std::ranges::find_if(
      bits_, [](const auto &bits) { return bits.Count() == 0; });
  return available != bits_.end() ? &*available : nullptr;
}

BitFormation *EnemyManager::FindBits(const EnemyActor &actor) {
  auto formation = std::ranges::find_if(
      bits_, [&actor](const auto &bits) { return bits.Owns(actor); });
  return formation != bits_.end() ? &*formation : nullptr;
}

const BitFormation *EnemyManager::FindBits(const EnemyActor &actor) const {
  auto formation = std::ranges::find_if(
      bits_, [&actor](const auto &bits) { return bits.Owns(actor); });
  return formation != bits_.end() ? &*formation : nullptr;
}
