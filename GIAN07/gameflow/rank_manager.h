/*
 *   RankManager — centralized play rank state and operations
 */

#pragma once

#include "PRankCtrl.h"

struct RankManager {
  PlayRankState state; // PlayRank

  // === メソッド ===
  void Add(int n);     // was PlayRankAdd
  void Reset();        // was PlayRankReset
};

extern RankManager Ranking;
