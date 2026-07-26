///
/// anime_data — per-stage enemy animation sprite sheet configuration
///
#pragma once

#include "core/game_manager.h"
#include "enemy/actor/enemy_actor.h"

namespace anime_data {

void SetupStageAnime(StageId stage, EnemyAnimationSet &animations);

} // namespace anime_data
