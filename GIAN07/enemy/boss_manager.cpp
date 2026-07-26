///
/// Boss actor lifecycle and subsystem orchestration
///

#include <algorithm>
#include <cstddef>
#include <format>
#include <utility>

#include "boss_manager.h"
#include "enemy_system.h"

#include "audio/snd.h"
#include "bullet/bullet_manager.h"
#include "core/gian.h"
#include "effect/bomb_efc.h"
#include "effect/effect_manager.h"
#include "gameflow/gameflow_manager.h"
#include "gfx/geometry.h"
#include "gfx/graphics_backend.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/stage_session.h"
#include "util/cast.h"
#include "util/ut_math.h"

static bool IsDefeated(const BossData &boss) {
  return boss.actor.hp == 0 && boss.actor.flag == EF_BOMB;
}

BossManager::BossManager(EnemySystem &system, BulletManager &bullets,
                         ItemManager &items, GameManager &game, Player &player,
                         stage::StageSession &stage)
    : system_(&system), bullets_(&bullets), items_(&items), game_(&game),
      player_(&player), stage_(&stage), snakes_(system, bullets),
      bits_(system, bullets, player) {}

// Initialize boss data (used on interrupt, stage clear)
void BossManager::Reset() {
  actors_.Init();

  // Boss is dead, so of course don't display HP gauge
  health_gauge_.Reset();

  // Initialize snake management (somewhat mysterious)
  snakes_.Reset();

  // Also initialize bit management
  bits_.Reset();
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
  system_->Execute(&boss->actor);

  const auto total_hp = TotalHp();
  if (open_health_gauge) {
    health_gauge_.Open(total_hp);
  } else {
    health_gauge_.AddPhase(total_hp);
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
    actor.IsDamaged = 0;
    UpdateActor(boss);

    // Cactus hit check //
    if (HITCHK(actor.x, player_->X(), actor.g_width) &&
        HITCHK(actor.y, player_->Y(), actor.g_height) &&
        player_->IsInvincible() == 0) {
      // Might be interesting to deal damage to the enemy around here? //
      if ((actor.flag & EF_HITSB) != 0) {
        player_->OnHit();
      }
    }

    // Prepare homing //
    if ((actor.flag & EF_DAMAGE) != 0) {
      system_->ConsiderHomingTarget(&actor);
    }

    // Sum total HP //
    HP_Sum += actor.hp;

    // Run animation //
    system_->UpdateAnimation(&actor);

    actor.count++;
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

  snakes_.Update();
  bits_.Update();
  health_gauge_.SetCombatState(phase_threshold_hp, timer_max, timer_now);
  health_gauge_.Update(HP_Sum);
}

// Draw the boss
void BossManager::DrawActors() {
  constexpr auto sid = SURFACE_ID::ENEMY;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  int t = 0;
  EnemyActor *e = nullptr;
  PIXEL_LTRB wing;

  bits_.DrawLinks();

  for (auto &boss : actors_) {
    auto *b = &boss;
    e = &(b->actor);

    x = (e->x >> 6);
    y = (e->y >> 6);

    // Spirit state //
    if (b->state == BossState::BombSpirit && (player_->IsBombActive() != 0U) &&
        ((e->flag & EF_DRAW) != 0)) {
      wing = PIXEL_LTWH{(160 + ((Cast::sign<int32_t>(e->count / 2) % 4) * 40)),
                        80, 40, 40};

      // pbg quirk: Blitted without clipping?! I'd consider this a
      // bug if it wasn't explicitly commented as such. Fine then...
      GrpBackend_SetClip(GRP_RES_RECT);

      // No clipping
      GrpSurface_Blit({(x - 20), (y - 20)}, sid, wing);

      GrpBackend_SetClip({X_MIN, Y_MIN, (X_MAX + 1), (Y_MAX + 1)});
      continue;
    }

    // Barrier state //
    if (b->state == BossState::BombShield && (player_->IsBombActive() != 0U) &&
        ((e->flag & EF_DRAW) != 0)) {
      GrpGeom->Lock();
      for (uint8_t j = 0; j <= 5; j++) {
        GrpGeom->SetColor({(5U - j), (5U - j), 5U});
        GeomCircle({x, y}, (sinl((e->count * 4), (30 + (j * 4))) + 80));
      }
      GrpGeom->Unlock();
    }

    switch (b->state) {
    case BossState::ButterflyWings:
      t = (b->state_frame - 64 - 8) << 2;
      t = std::max(t, 0);
      w = 64;
      h = 92;
      wing = {0, 176, 128, 360};
      GrpSurface_Blit({(x - w - t), (y - h)}, sid, wing);
      wing = {128, 176, 256, 360};
      GrpSurface_Blit({(x - w + t), (y - h)}, sid, wing);
      break;

    case BossState::BirdWings:
      w = 44;
      h = 52;
      wing = {552, 0, 640, 104};
      GrpSurface_Blit({(x - w - 50), (y - h)}, sid, wing);
      wing = {552, 104, 640, 208};
      GrpSurface_Blit({(x - w + 50), (y - h)}, sid, wing);
      break;
    case BossState::Normal:
    case BossState::BombShield:
    case BossState::BombSpirit:
      break;
    }

    if ((e->flag & EF_DRAW) != 0) {
      system_->DrawActor(*e);
    }
  }
}

// Boss enemy bullet clear preprocessing function
void BossManager::ClearProjectiles() { bits_.Destroy(); }

void BossManager::DrawHud(uint32_t stage_frame) {
  health_gauge_.Draw(stage_frame);
}

// Set HP of all currently active actors_ to 0
void BossManager::KillActors() {
  // Uses the same functions as damaging/destroying for fragment emission, etc.
  // // But naturally, score & experience? are not obtainable // Don't forget to
  // close lasers too!!                                       //

  for (auto &boss : actors_) {
    auto &actor = boss.actor;
    snakes_.Remove(boss);
    Effects.SpawnFragment(actor.x, actor.y, FRG_FATCIRCLE);
    Effects.SpawnBombEffect(actor.x, actor.y, EXBOMB_STD);
    Snd_SEPlay(SfxId::Bossbomb, actor.x);
    if (actor.LLaserRef != 0U) {
      bullets_->ControlLongLaser(
          &actor, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    actor.hp = 0;
    actor.count = 0;
    actor.flag = EF_BOMB;
  }

  actors_.Init();
}

bool BossManager::ApplyDamage(BossData &b, EnemyActor &e, int damage) {
  e.IsDamaged = ((e.count) & 1);
  if (std::cmp_less_equal(
          e.hp, damage)) { // Boss death processing (to be changed later!!)
    snakes_.Remove(b);
    bits_.Destroy();
    system_->ClearRegular();
    Effects.SpawnFragment(e.x, e.y, FRG_FATCIRCLE);
    Effects.SpawnBombEffect(e.x, e.y, EXBOMB_STD);
    stage_->Command(stage::BackgroundCommand::Quake, Effects);
    Snd_SEPlay(SfxId::Bossbomb, e.x);
    if (e.LLaserRef != 0U) {
      bullets_->ControlLongLaser(
          &e, ECL_ALL_LONG_LASERS,
          LongLaserUpdateInfo{
              LongLaserUpdateInfo::Command::ForceClose}); // Force close laser
    }
    player_->PowerUp(Cast::down<uint8_t>(e.hp));
    e.hp = 0;
    e.count = 0;
    e.flag = EF_BOMB;

    // If it was the last one //
    if (std::ranges::all_of(actors_, IsDefeated)) {
      const auto temp = bullets_->ScoreToItems(); // Bullet -> score effect
      // sprintf(buf, "%3d Evade  %5dPts", player_->GrazeCount(),
      // player_->evadesc);
      Effects.SpawnStringEffect(
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

// Deal damage to boss
bool BossManager::DamageAt(int x, int y, int damage) {
  int i = 0;
  EnemyActor *e = nullptr;

  i = (bits_.Count() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &boss : actors_) {
    if (boss.state != BossState::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    e = &(boss.actor);
    if (HITCHK(x, e->x, e->g_width) && HITCHK(y, e->y, e->g_height) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      const auto result = ApplyDamage(boss, *e, damage);
      RemoveDefeatedActors();
      return result;
    }
  }
  return false;
}

// Deal damage to boss (y-axis upward infinite ver.)
bool BossManager::DamageAt2(int x, int y, int damage) {
  int i = 0;
  EnemyActor *e = nullptr;
  bool ret_val = false;

  i = (bits_.Count() >> 1);
  damage -= i;
  if (damage <= 0) {
    return false;
  }

  for (auto &boss : actors_) {
    if (boss.state != BossState::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    e = &(boss.actor);
    if (HITCHK(x, e->x, e->g_width) && (y > e->y) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ret_val = ApplyDamage(boss, *e, damage);
    }
  }
  RemoveDefeatedActors();
  return ret_val;
}

// Deal damage to boss (diagonal laser)
void BossManager::DamageAt3(int x, int y, uint8_t d) {
  int i = 0;
  EnemyActor *e = nullptr;
  // BOOL			ret_val = FALSE;
  int damage = 2;

  i = (bits_.Count() >> 1);
  damage -= i;
  if (damage <= 0) {
    return;
  }

  for (auto &boss : actors_) {
    if (boss.state != BossState::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    e = &(boss.actor);
    if (EnemySystem::LaserHitCheck(e, x, y, d) &&
        ((e->flag & EF_DAMAGE) != 0)) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(boss, *e, damage);
    }
  }
  RemoveDefeatedActors();
}

// Deal damage to boss (all enemies)
void BossManager::DamageAll(int damage) {
  int i = 0;
  EnemyActor *e = nullptr;

  i = (bits_.Count() >> 1);
  damage -= i;
  if (damage <= 0) {
    return;
  }

  for (auto &boss : actors_) {
    if (boss.state != BossState::Normal && player_->IsBombActive() != 0U) {
      continue;
    }

    e = &(boss.actor);
    if ((e->flag & EF_DAMAGE) != 0) {
      if (e->flag == EF_BOMB || ((e->flag & EF_DAMAGE) == 0)) {
        {
          continue;
        }
      }
      ApplyDamage(boss, *e, damage);

      // return TRUE;
    }
  }
  RemoveDefeatedActors();
  //	return FALSE;
}

void BossManager::RemoveDefeatedActors() { actors_.Compact(IsDefeated); }

// Normal ECL-compatible movement
void BossManager::UpdateActor(BossData &boss) {
  EnemyActor *e = &boss.actor;

  // Normal enemy processing //
  system_->CheckInterrupts(e);
  system_->Execute(e);

  // Branch by bullet fire mode //
  if (e->t_rep != 0U) {
    e->tama_c = (e->tama_c + 1) % (e->t_rep);
    if (e->tama_c == 0) {
      auto si = MakeBulletSpawnInfo(e->t_cmd, e->x, e->y, true, *game_);
      bullets_->SpawnBullet(si);
    }
  }

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
void BossManager::HandleAction(EnemyActor *e, EclBossAction action) {
  auto b = std::ranges::find_if(
      actors_, [e](const auto &b) { return ((&b.actor) == e); });
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
    bits_.Spawn(*b, 5, 3);
    break;

  case EclBossAction::EnableSixBits:
    bits_.Spawn(*b, 6, 3);
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
void BossManager::SetBitAttack(EnemyActor *e, uint32_t script_id) {
  const auto b = std::ranges::find_if(
      actors_, [e](const auto &b) { return ((&b.actor) == e); });
  if (b == actors_.end()) {
    return;
  }

  bits_.SelectAttack(script_id);
}

// Set laser command to bit
void BossManager::ControlBitLaser(EnemyActor *e, EclBitLaserCommand command) {
  const auto b = std::ranges::find_if(
      actors_, [e](const auto &b) { return ((&b.actor) == e); });
  if (b == actors_.end()) {
    return;
  }

  bits_.LaserCommand(command);
}

// Send bit command
void BossManager::ControlBits(EnemyActor *e, EclBitCommand command, int param) {
  const auto b = std::ranges::find_if(
      actors_, [e](const auto &b) { return ((&b.actor) == e); });
  if (b == actors_.end()) {
    return;
  }

  bits_.Command(command, param);
}

// Set the SCL-level timeout for countdown fallback
void BossManager::SetStageTimeout(int32_t timeout_end) {
  health_gauge_.SetStageTimeout(timeout_end);
}

// Return remaining bit count
int BossManager::BitCount() const { return bits_.Count(); }
