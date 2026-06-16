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

// === 後方互換 inline wrapper ===
inline void PlayRankAdd(int n) { Ranking.Add(n); }
inline void PlayRankReset(void) { Ranking.Reset(); }
