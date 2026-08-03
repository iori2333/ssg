///
/// EnemyManager - owns regular enemies, bosses, and their shared runtime
///

#pragma once

#include <array>
#include <cstdint>

#include "actor/enemy_actor.h"
#include "boss/bit_formation.h"
#include "boss/boss.h"
#include "boss/snake_formation.h"
#include "ecl/ecl_host.h"
#include "ecl/ecl_vm.h"
#include "render/enemy_renderer.h"

#include "gfx/core/coords.h"
#include "util/object_pool.h"

struct BulletManager;
class EffectManager;
struct GameSession;
class ItemSystem;
struct PlayerAttack;
class Player;

namespace audio {
class AudioSystem;
}

namespace stage {
class StageSession;
}

struct EnemyHomingTarget {
  bool active = false;
  WorldCoord x{};
  WorldCoord y{};
};

class EnemyManager {
public:
  EnemyManager(BulletManager &bullets, ItemSystem &items, GameSession &session,
               Player &player, stage::StageSession &stage,
               EffectManager &effects, audio::AudioSystem &audio);

  [[nodiscard]] bool InstallStageAssets(EclProgram program,
                                        EnemyAnimationSet animations);

  void Reset();
  void ResetRegular();
  void Update();

  void DrawBosses();
  void DrawRegular();

  void SpawnFromScene(int x, int y, uint32_t script_id);
  void SpawnBoss(PixelPoint position, uint32_t script_id);
  void KillBosses();
  void SetBossTimeout(int timeout_end);

  [[nodiscard]] int BossCount() const {
    return static_cast<int>(bosses_.Size());
  }
  [[nodiscard]] uint32_t BossHpSum() const { return TotalBossHp(); }
  [[nodiscard]] const BossHudModel &BossHud() const { return boss_hud_; }
  [[nodiscard]] const EnemyHomingTarget &HomingTarget() const {
    return homing_target_;
  }
  void ResetHomingTarget();

  bool ApplyPlayerAttack(const PlayerAttack &attack);

private:
  friend class BitFormation;
  friend class EclHost;
  friend class SnakeFormation;

  // Shared actor operations.
  void BeginActorFrame(EnemyActor &actor, bool allow_fire_with_zero_hp);
  void CheckPlayerCollision(const EnemyActor &actor) const;
  void FinishActorFrame(EnemyActor &actor, bool consider_homing);
  void InitializeActor(EnemyActor &actor, WorldPoint position,
                       uint32_t script_id);
  void RetireActor(EnemyActor &actor);
  void ConsiderHomingTarget(const EnemyActor &actor);

  // Regular enemies.
  void ClearRegular();
  void CompactRegular();
  EnemyActor *SpawnRegular(WorldPoint position, uint32_t script_id);
  void UpdateRegular();
  void ApplyRegularDamage(EnemyActor &actor, int damage);

  // Bosses and their formations.
  void ResetBosses();
  void UpdateBosses();
  void ClearBossProjectiles();
  void SpawnBossActor(WorldPoint position, uint32_t script_id,
                      bool starts_encounter);
  void SpawnBossFromEcl(WorldPoint position, uint32_t script_id);
  void RemoveFinishedBosses();
  void OnActorRetired(const EnemyActor &actor);

  void ApplyBossDamage(BossActor &boss, int damage);
  bool ApplyPlayerAttackToBosses(const PlayerAttack &attack);
  [[nodiscard]] uint32_t TotalBossHp() const;

  void HandleBossAction(EnemyActor &actor, EclBossAction action);
  void SetBitAttack(EnemyActor &actor, uint32_t script_id);
  void ControlBitLaser(EnemyActor &actor, EclBitLaserCommand command);
  void ControlBits(EnemyActor &actor, EclBitCommand command, int parameter);
  [[nodiscard]] int BitCount(const EnemyActor &actor) const;
  [[nodiscard]] BitFormation *AcquireBits(BossActor &boss);
  [[nodiscard]] BitFormation *FindBits(const EnemyActor &actor);
  [[nodiscard]] const BitFormation *FindBits(const EnemyActor &actor) const;

  EnemyAnimationSet animations_{};
  EnemyRenderer renderer_;
  util::ObjectPool<EnemyActor, kEnemyCapacity> regular_enemies_;
  util::ObjectPool<BossActor, kBossCapacity> bosses_;
  BossHudModel boss_hud_{};

  EnemyHomingTarget homing_target_;
  WorldCoord homing_distance_ = kNoHomingDistance;

  BulletManager &bullets_;
  GameSession &session_;
  ItemSystem &items_;
  Player &player_;
  stage::StageSession &stage_;
  EffectManager &effects_;
  audio::AudioSystem &audio_;

  EclHost ecl_host_;
  EclVm ecl_;
  SnakeFormation snakes_;
  std::array<BitFormation, kBossCapacity> bits_;
};
