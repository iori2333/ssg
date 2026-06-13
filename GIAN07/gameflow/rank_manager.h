/*
 *   RankManager — centralized play rank state
 */

#pragma once

#include "PRankCtrl.h"

struct RankManager {
  PlayRankState state; // PlayRank
};

extern RankManager Ranking;
