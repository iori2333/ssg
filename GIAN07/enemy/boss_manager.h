///
/// BossManager - centralized boss system state and operations
///

#pragma once

#include <cstdint>

#include "bit_formation.h"
#include "boss.h"
#include "boss_health_gauge.h"
#include "snake_formation.h"

#include "core/object_pool.h"
#include "gfx/coords.h"

struct BulletManager;
class EnemySystem;
struct ItemManager;
struct GameManager;
class Player;

namespace stage {
class StageSession;
}

class BossManager {
  friend class EnemySystem;

  BossManager(EnemySystem &system, BulletManager &bullets, ItemManager &items,
              GameManager &game, Player &player, stage::StageSession &stage);

  ObjectPool<BossData, BOSS_MAX> actors_;
  BossHealthGauge health_gauge_;

  SnakeFormation snakes_;
  BitFormation bits_;

  EnemySystem *system_;
  BulletManager *bullets_;
  ItemManager *items_;
  GameManager *game_;
  Player *player_;
  stage::StageSession *stage_;

  // === Methods ===

  // Initialization and setup
  void Reset();
  void Spawn(WORLD_POINT position, uint32_t script_id);
  void SpawnFromEcl(WORLD_POINT position, uint32_t script_id);

  // Movement and drawing
  void Update();
  void DrawActors();
  void ClearProjectiles();
  void DrawHud(uint32_t stage_frame);

  // Timer
  void SetStageTimeout(int32_t timeout_end);

  // HP
  void KillActors();
  [[nodiscard]] uint16_t ActiveCount() const {
    return static_cast<uint16_t>(actors_.Size());
  }
  [[nodiscard]] uint32_t TotalHp() const;

  // Damage
  bool ApplyDamage(BossData &b, EnemyActor &e, int damage);
  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t d);
  void DamageAll(int damage);

  // Interrupts and bit control
  void HandleAction(EnemyActor *actor, EclBossAction action);
  void SetBitAttack(EnemyActor *actor, uint32_t script_id);
  void ControlBitLaser(EnemyActor *actor, EclBitLaserCommand command);
  void ControlBits(EnemyActor *actor, EclBitCommand command, int parameter);
  [[nodiscard]] int BitCount() const;

  void SpawnActor(WORLD_POINT position, uint32_t script_id,
                  bool open_health_gauge);
  void UpdateActor(BossData &boss);
  void RemoveDefeatedActors();
};
