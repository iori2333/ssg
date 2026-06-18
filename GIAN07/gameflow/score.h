///
/// Score - Score I/O functions
///

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

// [ Constants ]
inline constexpr std::size_t NR_NAME_LEN =
    9; // Name registry name length (incl. '\0')
inline constexpr std::size_t NR_RANK_MAX = 5; // Maximum ranked entries (Save)

// [ Structs ]

struct NrNameData {
  char Name[NR_NAME_LEN]; // Name
  int64_t Score;          // Score
  uint32_t Evade;         // Graze count
  uint8_t Stage;          // Stage
  uint8_t Weapon;         // Equipment
};
// (NrNameData alias removed — use NrNameData directly)

struct NrScoreData {
  NrNameData Easy[NR_RANK_MAX];    // Difficulty: Easy
  NrNameData Normal[NR_RANK_MAX];  // Difficulty: Normal
  NrNameData Hard[NR_RANK_MAX];    // Difficulty: Hard
  NrNameData Lunatic[NR_RANK_MAX]; // Difficulty: Lunatic
  NrNameData Extra[NR_RANK_MAX];   // Difficulty: Extra
};
// (NR_SCORE_DATA alias removed — use NrScoreData directly)

struct NrScoreString {
  uint8_t Rank;     // Actual rank (handles ties)
  int x, y;         // Drawing coordinates
  bool bMoveEnable; // Movable?

  char Name[NR_NAME_LEN];          // Name
  std::string Score;                // Score
  std::string Evade;                // Graze
  std::string Stage;                // Stage
  uint8_t Weapon;                   // Equipment
};
// (NR_SCORE_STRING alias removed — use NrScoreString directly)

// [ Functions ]
// Backward-compat inline wrappers moved to score_manager.h
// Implementations migrated to ScoreManager methods
