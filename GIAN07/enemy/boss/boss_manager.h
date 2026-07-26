///
/// BossManager - centralized boss system state and operations
///

#pragma once

#include <array>
#include <cstdint>

#include "bit_formation.h"
#include "boss.h"
#include "snake_formation.h"

#include "core/object_pool.h"
#include "enemy/actor/enemy_combat.h"
#include "gfx/coords.h"

struct BulletManager;
class EnemySystem;
class EclHost;
struct EffectManager;
struct ItemManager;
class Player;

namespace stage {
class StageSession;
}

class BossManager {
  friend class EclHost;
  friend class EnemySystem;

  BossManager(EnemySystem &system, BulletManager &bullets, ItemManager &items,
              Player &player, stage::StageSession &stage,
              EffectManager &effects);

  EnemySystem *system_;
  BulletManager *bullets_;
  ItemManager *items_;
  Player *player_;
  stage::StageSession *stage_;
  EffectManager *effects_;

  ObjectPool<BossData, BOSS_MAX> actors_;
  BossHudModel hud_{};

  SnakeFormation snakes_;
  std::array<BitFormation, BOSS_MAX> bits_;

  // === Methods ===

  // Initialization and setup
  void Reset();
  void OnActorRetired(const EnemyActor &actor);
  void Spawn(WORLD_POINT position, uint32_t script_id);
  void SpawnFromEcl(WORLD_POINT position, uint32_t script_id);

  // Movement and drawing
  void Update();
  void ClearProjectiles();

  // Timer
  void SetStageTimeout(int32_t timeout_end);

  // HP
  void KillActors();
  [[nodiscard]] uint16_t ActiveCount() const {
    return static_cast<uint16_t>(actors_.Size());
  }
  [[nodiscard]] uint32_t TotalHp() const;
  [[nodiscard]] const BossHudModel &Hud() const { return hud_; }

  // Damage
  bool ApplyDamage(BossData &boss, int damage);
  bool ApplyAttack(const EnemyAttack &attack);

  // Interrupts and bit control
  void HandleAction(EnemyActor &actor, EclBossAction action);
  void SetBitAttack(EnemyActor &actor, uint32_t script_id);
  void ControlBitLaser(EnemyActor &actor, EclBitLaserCommand command);
  void ControlBits(EnemyActor &actor, EclBitCommand command, int parameter);
  [[nodiscard]] int BitCount(const EnemyActor &actor) const;

  void SpawnActor(WORLD_POINT position, uint32_t script_id,
                  bool open_health_gauge);
  void UpdateState(BossData &boss);
  void RemoveFinishedActors();
  [[nodiscard]] BitFormation *AcquireBits(BossData &boss);
  [[nodiscard]] BitFormation *FindBits(const EnemyActor &actor);
  [[nodiscard]] const BitFormation *FindBits(const EnemyActor &actor) const;
};
