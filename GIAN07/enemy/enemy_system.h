///
/// EnemySystem - owns the active ECL program, regular enemies, and bosses
///

#pragma once

#include <cstdint>

#include "actor/enemy_actor.h"
#include "actor/enemy_actor_runtime.h"
#include "actor/enemy_combat.h"
#include "boss/boss_manager.h"
#include "ecl/ecl_host.h"
#include "ecl/ecl_vm.h"
#include "render/enemy_renderer.h"

#include "core/object_pool.h"
#include "gfx/coords.h"

struct BulletManager;
struct EffectManager;
struct GameManager;
struct ItemManager;
class Player;

namespace stage {
class StageSession;
}

struct EnemyHomingTarget {
  bool active = false;
  int x = 0;
  int y = 0;
};

class EnemySystem {
public:
  EnemySystem(BulletManager &bullets, ItemManager &items, GameManager &game,
              Player &player, stage::StageSession &stage,
              EffectManager &effects);

  [[nodiscard]] bool InstallStageAssets(EclProgram program,
                                        EnemyAnimationSet animations);

  void Reset();
  void ResetRegular();
  void Update();

  void DrawBosses();
  void DrawRegular();

  void SpawnFromScene(int16_t x, int16_t y, uint8_t script_id);
  void SpawnBoss(PIXEL_POINT position, uint32_t script_id);
  void KillBosses();
  void SetBossTimeout(int32_t timeout_end);

  [[nodiscard]] uint16_t BossCount() const { return bosses_.ActiveCount(); }
  [[nodiscard]] uint32_t BossHpSum() const { return bosses_.TotalHp(); }
  [[nodiscard]] const BossHudModel &BossHud() const { return bosses_.Hud(); }
  [[nodiscard]] const EnemyHomingTarget &HomingTarget() const {
    return homing_target_;
  }
  void ResetHomingTarget();

  bool ApplyAttack(const EnemyAttack &attack);

private:
  friend class BossManager;
  friend class BitFormation;
  friend class EclHost;
  friend class EnemyActorRuntime;
  friend class SnakeFormation;

  // Shared actor/ECL operations used by the owned BossManager.
  void ClearRegular();
  void CompactRegular();
  void RetireActor(EnemyActor &actor);
  EnemyActor *SpawnRegular(WORLD_POINT position, uint32_t script_id);
  void InitializeActor(EnemyActor &actor, WORLD_POINT position,
                       uint32_t script_id);
  void UpdateAnimation(EnemyActor &actor);
  void ConsiderHomingTarget(const EnemyActor &actor);

  void MoveRegular();
  bool ApplyDamage(EnemyActor &actor, int damage);

  EnemyAnimationSet animations_{};
  EnemyRenderer renderer_;
  ObjectPool<EnemyActor, ENEMY_MAX> regular_enemies_;
  EnemyHomingTarget homing_target_;
  int homing_distance_ = HOMING_DUMMY;

  BulletManager *bullets_;
  ItemManager *items_;
  Player *player_;

  EclHost ecl_host_;
  EclVm ecl_;
  EnemyActorRuntime actor_runtime_;
  BossManager bosses_;
};
