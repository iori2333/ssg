///
/// Boss.h   Boss logic (includes mid-bosses)
///

#pragma once

#include "enemy.h"
#include <array>
#include <cstdint>

// [ Structs ]

// Special hit detection
struct ExHitCheck {
  uint8_t flags[60][60];
};
// (EXHITCHK alias removed — use ExHitCheck directly)

// Boss data
struct BossData {
  EnemyData Edat;  // Standard enemy data (note: this is the entity)
  ExHitCheck *Hit; // Special hit detection (null if unused)

  void (*ExMove)(BossData *); // Special movement function

  uint32_t ExCount;    // State counter (zeroed on transition)
  uint8_t ExState;     // Special state
  bool IsUsed = false; // Is this data in use?
};
// (BOSS_DATA alias removed — use BossData directly)

// [ Constants ]
inline constexpr auto BOSS_MAX = 4;        // Maximum number of bosses
inline constexpr auto BOSSHPG_HEIGHT = 24; // HP gauge height

// Boss HP gauge
struct BossHpgInfo {
  uint32_t Now, Max; // Current & max HP
  uint32_t Next;     // Next HP value
  uint32_t Update;   // Update value
  uint32_t Count;    // Frame count

  std::array<int, BOSSHPG_HEIGHT> XTemp; // HP gauge effect temp
  uint8_t State;                         // State

  int32_t PhaseThresholdHp = -1; // Next HP threshold from first boss (-1 = none)
  int32_t TimerMax = -1;         // Vect[ECLVECT_TIMER].value (-1 = no timer)
  int32_t TimerNow = 0;          // Current IntTimer from first boss
  int32_t PrevTimerSeconds = -1; // For detecting second changes
  int32_t SCLTimerEnd = -1;      // SCL TIME target game_count for fallback countdown
};

// [ Functions ]
// Backward-compat inline wrappers moved to boss_manager.h
// Implementations migrated to BossManager methods

// [ Variables ]
// Access directly via Bosses.bosses, Bosses.count, Bosses.hpg
