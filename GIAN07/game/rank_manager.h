/*
 *   RankManager — centralized play rank state
 */

#pragma once

#include "game/PRankCtrl.h"

struct RankManager {
  PlayRankState state; // PlayRank
};

extern RankManager Ranking;
