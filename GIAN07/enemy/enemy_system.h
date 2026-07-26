///
/// EnemySystem - owns the active ECL program, regular enemies, and bosses
///

#pragma once

#include <cstdint>
#include <utility>

#include "boss_manager.h"
#include "ecl_program.h"
#include "enemy.h"

#include "core/object_pool.h"
#include "gfx/coords.h"

struct BulletManager;
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
              Player &player, stage::StageSession &stage);

  void InstallStageAssets(EclProgram program, EnemyAnimationSet animations) {
    program_ = std::move(program);
    anime = std::move(animations);
  }

  void Reset();
  void ResetRegular();
  void Update();

  void DrawBosses();
  void DrawRegular();
  void DrawBossHud(uint32_t stage_frame);

  void SpawnFromScene(int16_t x, int16_t y, uint8_t script_id);
  void SpawnBoss(PIXEL_POINT position, uint32_t script_id);
  void KillBosses();
  void SetBossTimeout(int32_t timeout_end);

  [[nodiscard]] uint16_t BossCount() const { return bosses_.ActiveCount(); }
  [[nodiscard]] uint32_t BossHpSum() const { return bosses_.TotalHp(); }
  [[nodiscard]] const EnemyHomingTarget &HomingTarget() const {
    return homing_target_;
  }
  void ResetHomingTarget();

  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t direction);
  void DamageAll(int damage);

private:
  friend class BossManager;
  friend class BitFormation;
  friend class SnakeFormation;

  // Shared actor/ECL operations used by the owned BossManager.
  void ClearRegular();
  void DrawActor(const EnemyActor &actor) const;
  EnemyActor *SpawnRegular(WORLD_POINT position, uint32_t script_id);
  void InitializeActor(EnemyActor &actor, WORLD_POINT position,
                       uint32_t script_id);
  void JumpToScript(EnemyActor *actor, uint32_t script_id);
  void Execute(EnemyActor *actor);
  enum class EclStep { Advance, Jump, Yield, Repeat, Halt };
  EclStep ExecuteInstruction(EnemyActor &actor,
                             const EclInstruction &instruction,
                             int &comparison);
  EclStep ExecuteControlInstruction(EnemyActor &actor,
                                    const EclInstruction &instruction);
  EclStep ExecuteMovementInstruction(EnemyActor &actor,
                                     const EclInstruction &instruction);
  EclStep ExecuteBulletInstruction(EnemyActor &actor,
                                   const EclInstruction &instruction);
  EclStep ExecuteLaserInstruction(EnemyActor &actor,
                                  const EclInstruction &instruction);
  EclStep ExecuteActorInstruction(EnemyActor &actor,
                                  const EclInstruction &instruction);
  EclStep ExecuteRegisterInstruction(EnemyActor &actor,
                                     const EclInstruction &instruction,
                                     int &comparison);
  [[nodiscard]] static uint32_t ReadValue(const EnemyActor &actor,
                                          EclValue value);
  static void WriteValue(EnemyActor &actor, EclValue destination,
                         uint32_t value);
  void CheckInterrupts(EnemyActor *actor);
  void UpdateAnimation(EnemyActor *actor);
  void ConsiderHomingTarget(const EnemyActor *actor);

  static bool LaserHitCheck(const EnemyActor *actor, int origin_x, int origin_y,
                            uint8_t direction);

  void MoveRegular();
  bool ApplyDamage(EnemyActor &actor, int damage);

  ObjectPool<EnemyActor, ENEMY_MAX> regular_enemies_;

  EclProgram program_;
  EnemyAnimationSet anime{};
  EnemyHomingTarget homing_target_;
  int homing_distance_ = HOMING_DUMMY;

  uint8_t enemy_exdeg = 0;
  uint8_t enemy_exdeg_d = 0;

  BulletManager *bullets_;
  ItemManager *items_;
  GameManager *game_;
  Player *player_;
  stage::StageSession *stage_;

  BossManager bosses_;
};
