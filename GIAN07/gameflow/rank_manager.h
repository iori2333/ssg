///
/// RankManager - Centralized play rank state and operations
///

#pragma once

#include "play_rank.h"

struct RankManager {
  PlayRankState state; // PlayRank

  // === Methods ===
  void Add(int n);
  void Reset();
};

extern RankManager Ranking;
