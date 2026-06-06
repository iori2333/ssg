/*
 *   RankManager — centralized play rank state
 */

#include "game/rank_manager.h"
#include "game/PRankCtrl.h"

RankManager Ranking;
PlayRankState &PlayRank = Ranking.state;
