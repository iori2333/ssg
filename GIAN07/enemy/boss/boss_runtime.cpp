///
/// Boss actor lifecycle and subsystem orchestration
///

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/gian.h"
#include "effect/bomb_efc.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "player/player_attack.h"
#include "stage/stage_session.h"
#include "util/cast.h"
#include "util/ut_math.h"

// Initialize boss data (used on interrupt, stage clear)
void EnemyManager::ResetBosses() {
  snakes_.Reset();
  for (auto &bits : bits_) {
    bits.Reset();
  }

  for (auto &boss : bosses_) {
    RetireActor(boss);
  }
  bosses_.Init();

  const auto next_revision = boss_hud_.encounter_revision + 1;
  boss_hud_ = {.encounter_revision = next_revision};
}

// Start a new boss encounter.
// Add a boss from an ECL command without reopening the HUD frame.
void EnemyManager::SpawnBossFromEcl(WORLD_POINT position, uint32_t script_id) {
  SpawnBossActor(position, script_id, false);
}

void EnemyManager::SpawnBossActor(WORLD_POINT position, uint32_t script_id,
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
// Move the boss
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

  int32_t phase_threshold_hp = -1;
  int32_t timer_max = -1;
  int32_t timer_now = 0;
  for (const auto &boss : bosses_) {
    const auto &hp_interrupt =
        boss.script.interrupts[static_cast<size_t>(EclInterrupt::Hp)];
    const auto &timer_interrupt =
        boss.script.interrupts[static_cast<size_t>(EclInterrupt::Timer)];
    if (phase_threshold_hp < 0 && hp_interrupt.target) {
      phase_threshold_hp = static_cast<int32_t>(hp_interrupt.threshold);
    }
    if (timer_max < 0 && timer_interrupt.target) {
      timer_max = static_cast<int32_t>(timer_interrupt.threshold);
      timer_now = static_cast<int32_t>(boss.script.interrupt_timer);
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

// Boss enemy bullet clear preprocessing function
void EnemyManager::ClearBossProjectiles() {
  for (auto &bits : bits_) {
    bits.Destroy();
  }
}

// Set HP of all currently active actors_ to 0
void EnemyManager::KillBosses() {
  // Uses the same functions as damaging/destroying for fragment emission, etc.
  // // But naturally, score & experience? are not obtainable // Don't forget to
  // close lasers too!!                                       //

  for (auto &bits : bits_) {
    bits.Destroy();
  }

  for (auto &boss : bosses_) {
    snakes_.Remove(boss);
    effects_->SpawnFragment(boss.x, boss.y, FRG_FATCIRCLE);
    effects_->SpawnBombEffect(boss.x, boss.y, EXBOMB_STD);
    Snd_SEPlay(SfxId::Bossbomb, boss.x);
    if (boss.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &boss, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    boss.BeginExplosion();
    RetireActor(boss);
  }

  bosses_.Init();
}

void EnemyManager::ApplyBossDamage(BossActor &boss, int damage) {
  EnemyActor &actor = boss;
  actor.damage_flash = actor.count & 1;
  if (std::cmp_less_equal(
          actor.hp, damage)) { // Boss death processing (to be changed later!!)
    snakes_.Remove(boss);
    if (auto *bits = FindBits(actor)) {
      bits->Destroy();
    }
    ClearRegular();
    effects_->SpawnFragment(actor.x, actor.y, FRG_FATCIRCLE);
    effects_->SpawnBombEffect(actor.x, actor.y, EXBOMB_STD);
    stage_->Command(stage::BackgroundCommand::Quake, *effects_);
    Snd_SEPlay(SfxId::Bossbomb, actor.x);
    if (actor.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &actor, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(Cast::down<uint8_t>(actor.hp));
    actor.BeginExplosion();

    // If it was the last one //
    if (std::ranges::all_of(
            bosses_, [](const auto &boss) { return boss.IsDefeated(); })) {
      const auto temp = bullets_->ScoreToItems(); // Bullet -> score effect
      // sprintf(buf, "%3d Evade  %5dPts", player_->GrazeCount(),
      // player_->evadesc);
      effects_->SpawnStringEffect(
          180, 60, std::format("  Bonus    {:7}Pts", temp).c_str());
      player_->AddScore(temp);
    }

    if (actor.item != 0U) {
      items_->Spawn(actor.x, actor.y, actor.item);
    }
    player_->AddScore(actor.score);
    bullets_->Clear();
  } else {
    Snd_SEPlay(SfxId::Hit, actor.x);
    player_->PowerUp(damage);
    actor.hp -= damage;
  }
}

bool EnemyManager::ApplyPlayerAttackToBosses(const PlayerAttack &attack) {
  bool hit = false;
  for (auto &boss : bosses_) {
    if (boss.mode != BossMode::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    if (boss.state != EnemyActorState::Active || (boss.flag & EF_DAMAGE) == 0 ||
        !boss.IsHitBy(attack)) {
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

// Get the sum of all boss HP
uint32_t EnemyManager::TotalBossHp() const {
  uint32_t total = 0;
  for (const auto &boss : bosses_) {
    total += boss.hp;
  }
  return total;
}

// Boss interrupt processing
void EnemyManager::HandleBossAction(EnemyActor &actor, EclBossAction action) {
  auto b = std::ranges::find_if(bosses_, [&actor](auto &boss) {
    return static_cast<EnemyActor *>(&boss) == &actor;
  });
  if (b == bosses_.end()) {
    return;
  }

  // Branch by interrupt number //
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

// Bit attack address specification
void EnemyManager::SetBitAttack(EnemyActor &actor, uint32_t script_id) {
  if (auto *bits = FindBits(actor)) {
    bits->SelectAttack(script_id);
  }
}

// Set laser command to bit
void EnemyManager::ControlBitLaser(EnemyActor &actor,
                                   EclBitLaserCommand command) {
  if (auto *bits = FindBits(actor)) {
    bits->LaserCommand(command);
  }
}

// Send bit command
void EnemyManager::ControlBits(EnemyActor &actor, EclBitCommand command,
                               int param) {
  if (auto *bits = FindBits(actor)) {
    bits->Command(command, param);
  }
}

// Set the SCL-level timeout for countdown fallback
void EnemyManager::SetBossTimeout(int32_t timeout_end) {
  boss_hud_.stage_timeout_end = timeout_end;
}

// Return remaining bit count
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
