///
/// PlayRank - Play rank management
///

#pragma once

#include <cstdint>

#include "core/level.h"

// [Struct]
struct PlayRankState {
  GameLevel level; // Difficulty change also related to direction count
  int Rank;            // Value related to bullet speed changes
};
using PlayRankInfo = PlayRankState;

// [Global variables]

// [Functions]
// Migrated to RankManager
