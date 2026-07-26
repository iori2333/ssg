///
/// Boss actor lifecycle and subsystem orchestration
///

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

#include "boss_manager.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/gian.h"
#include "effect/bomb_efc.h"
#include "effect/effect_manager.h"
#include "enemy/enemy_system.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/stage_session.h"
#include "util/cast.h"
#include "util/ut_math.h"

static bool IsDefeated(const BossData &boss) {
  return boss.actor.hp == 0 && boss.actor.state == EnemyActorState::Exploding;
}

static bool IsFinished(const BossData &boss) {
  return boss.actor.state == EnemyActorState::PendingRemoval ||
         IsDefeated(boss);
}

BossManager::BossManager(EnemySystem &system, BulletManager &bullets,
                         ItemManager &items, Player &player,
                         stage::StageSession &stage, EffectManager &effects)
    : system_(&system), bullets_(&bullets), items_(&items), player_(&player),
      stage_(&stage), effects_(&effects), snakes_(system, bullets),
      bits_{BitFormation(system, bullets, player),
            BitFormation(system, bullets, player),
            BitFormation(system, bullets, player),
            BitFormation(system, bullets, player)} {}

// Initialize boss data (used on interrupt, stage clear)
void BossManager::Reset() {
  snakes_.Reset();
  for (auto &bits : bits_) {
    bits.Reset();
  }

  for (auto &boss : actors_) {
    system_->RetireActor(boss.actor);
  }
  actors_.Init();

  const auto next_revision = hud_.encounter_revision + 1;
  hud_ = {.encounter_revision = next_revision};
}

void BossManager::OnActorRetired(const EnemyActor &actor) {
  snakes_.OnActorRetired(actor);
  for (auto &bits : bits_) {
    bits.OnActorRetired(actor);
  }
}

// Start a new boss encounter.
void BossManager::Spawn(WORLD_POINT position, uint32_t script_id) {
  SpawnActor(position, script_id, true);
}

// Add a boss from an ECL command without reopening the HUD frame.
void BossManager::SpawnFromEcl(WORLD_POINT position, uint32_t script_id) {
  SpawnActor(position, script_id, false);
}

void BossManager::SpawnActor(WORLD_POINT position, uint32_t script_id,
                             bool open_health_gauge) {
  auto *boss = actors_.Alloc();
  if (boss == nullptr) {
    return;
  }

  system_->InitializeActor(boss->actor, position, script_id);
  boss->actor.item = 0;
  boss->state_frame = 0;
  boss->state = BossState::Normal;
  system_->ecl_.Execute(boss->actor);

  const auto total_hp = TotalHp();
  if (open_health_gauge) {
    ++hud_.encounter_revision;
    hud_.phase_revision = 1;
    hud_.active = true;
    hud_.max_hp = total_hp;
    hud_.phase_hp = total_hp;
    hud_.stage_timeout_end = -1;
  } else {
    ++hud_.phase_revision;
    hud_.phase_hp = total_hp;
  }
}
// Move the boss
void BossManager::Update() {
  uint32_t HP_Sum = 0;

  // A boss ECL block can spawn another boss. Re-read the pool size so the new
  // actor retains the established same-frame update behavior.
  for (std::size_t index = 0; index < actors_.Size(); ++index) {
    auto &boss = actors_.Active(index);
    auto &actor = boss.actor;
    system_->actor_runtime_.BeginFrame(actor, AutoFirePolicy::IgnoreHp);
    if (actor.state == EnemyActorState::Active) {
      UpdateState(boss);
      system_->actor_runtime_.CheckPlayerCollision(actor);
      HP_Sum += actor.hp;
    }

    system_->actor_runtime_.FinishFrame(actor, true);
  }

  int32_t phase_threshold_hp = -1;
  int32_t timer_max = -1;
  int32_t timer_now = 0;
  for (const auto &boss : actors_) {
    const auto &actor = boss.actor;
    const auto &hp_interrupt =
        actor.script.interrupts[static_cast<size_t>(EclInterrupt::Hp)];
    const auto &timer_interrupt =
        actor.script.interrupts[static_cast<size_t>(EclInterrupt::Timer)];
    if (phase_threshold_hp < 0 && hp_interrupt.target) {
      phase_threshold_hp = static_cast<int32_t>(hp_interrupt.threshold);
    }
    if (timer_max < 0 && timer_interrupt.target) {
      timer_max = static_cast<int32_t>(timer_interrupt.threshold);
      timer_now = static_cast<int32_t>(actor.script.interrupt_timer);
    }
  }

  RemoveFinishedActors();
  snakes_.Update();
  for (auto &bits : bits_) {
    bits.Update();
  }
  hud_.active = !actors_.IsEmpty();
  hud_.current_hp = HP_Sum;
  hud_.phase_threshold_hp = phase_threshold_hp;
  hud_.timer_max = timer_max;
  hud_.timer_now = timer_now;
}

// Boss enemy bullet clear preprocessing function
void BossManager::ClearProjectiles() {
  for (auto &bits : bits_) {
    bits.Destroy();
  }
}

// Set HP of all currently active actors_ to 0
void BossManager::KillActors() {
  // Uses the same functions as damaging/destroying for fragment emission, etc.
  // // But naturally, score & experience? are not obtainable // Don't forget to
  // close lasers too!!                                       //

  for (auto &bits : bits_) {
    bits.Destroy();
  }

  for (auto &boss : actors_) {
    auto &actor = boss.actor;
    snakes_.Remove(boss);
    effects_->SpawnFragment(actor.x, actor.y, FRG_FATCIRCLE);
    effects_->SpawnBombEffect(actor.x, actor.y, EXBOMB_STD);
    Snd_SEPlay(SfxId::Bossbomb, actor.x);
    if (actor.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &actor, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    actor.hp = 0;
    actor.count = 0;
    actor.state = EnemyActorState::Exploding;
    system_->RetireActor(actor);
  }

  actors_.Init();
}

bool BossManager::ApplyDamage(BossData &boss, int damage) {
  auto &e = boss.actor;
  e.damage_flash = e.count & 1;
  if (std::cmp_less_equal(
          e.hp, damage)) { // Boss death processing (to be changed later!!)
    snakes_.Remove(boss);
    if (auto *bits = FindBits(e)) {
      bits->Destroy();
    }
    system_->ClearRegular();
    effects_->SpawnFragment(e.x, e.y, FRG_FATCIRCLE);
    effects_->SpawnBombEffect(e.x, e.y, EXBOMB_STD);
    stage_->Command(stage::BackgroundCommand::Quake, *effects_);
    Snd_SEPlay(SfxId::Bossbomb, e.x);
    if (e.long_laser_count != 0U) {
      bullets_->ControlLongLaser(
          &e, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(Cast::down<uint8_t>(e.hp));
    e.hp = 0;
    e.count = 0;
    e.state = EnemyActorState::Exploding;

    // If it was the last one //
    if (std::ranges::all_of(actors_, IsDefeated)) {
      const auto temp = bullets_->ScoreToItems(); // Bullet -> score effect
      // sprintf(buf, "%3d Evade  %5dPts", player_->GrazeCount(),
      // player_->evadesc);
      effects_->SpawnStringEffect(
          180, 60, std::format("  Bonus    {:7}Pts", temp).c_str());
      player_->AddScore(temp);
    }

    if (e.item != 0U) {
      items_->Spawn(e.x, e.y, e.item);
    }
    player_->AddScore(e.score);
    bullets_->Clear();
  } else {
    Snd_SEPlay(SfxId::Hit, e.x);
    player_->PowerUp(damage);
    e.hp -= damage;
  }
  return true;
}

bool BossManager::ApplyAttack(const EnemyAttack &attack) {
  bool hit = false;
  for (auto &boss : actors_) {
    if (boss.state != BossState::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    auto &actor = boss.actor;
    if (actor.state != EnemyActorState::Active ||
        (actor.flag & EF_DAMAGE) == 0 || !EnemyAttackHits(actor, attack)) {
      continue;
    }
    const int damage = attack.boss_damage - (BitCount(actor) >> 1);
    if (damage <= 0) {
      continue;
    }
    hit = ApplyDamage(boss, damage);
    if (attack.first_hit_only) {
      break;
    }
  }
  RemoveFinishedActors();
  return hit;
}

void BossManager::RemoveFinishedActors() {
  for (auto &boss : actors_) {
    if (!IsFinished(boss)) {
      continue;
    }
    snakes_.Remove(boss);
    if (auto *bits = FindBits(boss.actor)) {
      bits->Destroy();
    }
    system_->RetireActor(boss.actor);
  }
  actors_.Compact(IsFinished);
}

// Normal ECL-compatible movement
void BossManager::UpdateState(BossData &boss) {
  switch (boss.state) {
  case BossState::ButterflyWings:
    if (boss.state_frame < 64 + 16 + 8) {
      boss.state_frame++;
    }
    break;
  case BossState::Normal:
  case BossState::BirdWings:
  case BossState::BombShield:
  case BossState::BombSpirit:
    break;
  }
}

// Get the sum of all boss HP
uint32_t BossManager::TotalHp() const {
  uint32_t total = 0;
  for (const auto &boss : actors_) {
    total += boss.actor.hp;
  }
  return total;
}

// Boss interrupt processing
void BossManager::HandleAction(EnemyActor &actor, EclBossAction action) {
  auto b = std::ranges::find_if(
      actors_, [&actor](const auto &boss) { return &boss.actor == &actor; });
  if (b == actors_.end()) {
    return;
  }

  // Branch by interrupt number //
  switch (action) {
  case EclBossAction::EnableSnake:
    snakes_.Spawn(*b, 11);
    break;

  case EclBossAction::ButterflyWings: // Also draw butterfly wings
    b->state = BossState::ButterflyWings;
    b->state_frame = 0;
    break;

  case EclBossAction::BirdWings: // Also draw bird wings
    b->state = BossState::BirdWings;
    b->state_frame = 0;
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
    b->state = BossState::BombShield;
    break;

  case EclBossAction::BombSpirit:
    b->state = BossState::BombSpirit;
    break;
  }
}

// Bit attack address specification
void BossManager::SetBitAttack(EnemyActor &actor, uint32_t script_id) {
  if (auto *bits = FindBits(actor)) {
    bits->SelectAttack(script_id);
  }
}

// Set laser command to bit
void BossManager::ControlBitLaser(EnemyActor &actor,
                                  EclBitLaserCommand command) {
  if (auto *bits = FindBits(actor)) {
    bits->LaserCommand(command);
  }
}

// Send bit command
void BossManager::ControlBits(EnemyActor &actor, EclBitCommand command,
                              int param) {
  if (auto *bits = FindBits(actor)) {
    bits->Command(command, param);
  }
}

// Set the SCL-level timeout for countdown fallback
void BossManager::SetStageTimeout(int32_t timeout_end) {
  hud_.stage_timeout_end = timeout_end;
}

// Return remaining bit count
int BossManager::BitCount(const EnemyActor &actor) const {
  const auto *bits = FindBits(actor);
  return bits != nullptr ? bits->Count() : 0;
}

BitFormation *BossManager::AcquireBits(BossData &boss) {
  if (auto *bits = FindBits(boss.actor)) {
    return bits;
  }
  auto available = std::ranges::find_if(
      bits_, [](const auto &bits) { return bits.Count() == 0; });
  return available != bits_.end() ? &*available : nullptr;
}

BitFormation *BossManager::FindBits(const EnemyActor &actor) {
  auto formation = std::ranges::find_if(
      bits_, [&actor](const auto &bits) { return bits.Owns(actor); });
  return formation != bits_.end() ? &*formation : nullptr;
}

const BitFormation *BossManager::FindBits(const EnemyActor &actor) const {
  auto formation = std::ranges::find_if(
      bits_, [&actor](const auto &bits) { return bits.Owns(actor); });
  return formation != bits_.end() ? &*formation : nullptr;
}
