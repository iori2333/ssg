/*
 *   RankManager — centralized play rank state
 */

#include "game/rank_manager.h"

RankManager Ranking;
PlayRankState &PlayRank = Ranking.state;
