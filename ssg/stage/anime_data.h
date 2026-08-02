///
/// anime_data — per-stage enemy animation sprite sheet configuration
///
#pragma once

#include "enemy/actor/enemy_actor.h"
#include "gameplay/game_rules.h"

namespace anime_data {

void SetupStageAnime(StageId stage, EnemyAnimationSet &animations);

} // namespace anime_data
