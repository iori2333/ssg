///
/// PlayRank - Play rank management
///

#pragma once

#include <cstdint>

// [Struct]
struct PlayRankState {
  uint8_t GameLevel; // Difficulty change also related to direction count
  int Rank;          // Value related to bullet speed changes
};
using PlayRankInfo = PlayRankState;

// [Global variables]

// [Functions]
// Migrated to RankManager
