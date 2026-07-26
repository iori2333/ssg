///
/// EnemySystem - enemy subsystem ownership and cross-component orchestration
///

#include "enemy_system.h"

#include "bullet/bullet_manager.h"
#include "core/game_manager.h"
#include "item/item_manager.h"
#include "player/player.h"
#include "stage/stage_session.h"

EnemySystem::EnemySystem(BulletManager &bullets, ItemManager &items,
                         GameManager &game, Player &player,
                         stage::StageSession &stage)
    : bullets_(&bullets), items_(&items), game_(&game), player_(&player),
      stage_(&stage), bosses_(*this, bullets, items, game, player, stage) {
  Reset();
}

void EnemySystem::Reset() {
  bosses_.Reset();
  ResetRegular();
  ResetHomingTarget();
}

void EnemySystem::Update() {
  ResetHomingTarget();
  bosses_.Update();
  MoveRegular();
}

void EnemySystem::ResetHomingTarget() {
  homing_target_ = {};
  homing_distance_ = HOMING_DUMMY;
}

void EnemySystem::DrawBosses() { bosses_.DrawActors(); }

void EnemySystem::DrawBossHud(uint32_t stage_frame) {
  bosses_.DrawHud(stage_frame);
}

void EnemySystem::SpawnBoss(PIXEL_POINT position, uint32_t script_id) {
  bosses_.Spawn(WORLD_POINT{position}, script_id);
}

void EnemySystem::KillBosses() { bosses_.KillActors(); }

void EnemySystem::SetBossTimeout(int32_t timeout_end) {
  bosses_.SetStageTimeout(timeout_end);
}
