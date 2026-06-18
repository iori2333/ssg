///
/// Level - Difficulty & option settings
///

#pragma once

#include <cstdint>

inline constexpr uint8_t GAME_EASY = 0;    // Difficulty: Easy
inline constexpr uint8_t GAME_NORMAL = 1;  // Difficulty: Normal
inline constexpr uint8_t GAME_HARD = 2;    // Difficulty: Hard
inline constexpr uint8_t GAME_LUNATIC = 3; // Difficulty: Lunatic
inline constexpr uint8_t GAME_EXTRA = 4;   // Extra stage...

// Extra Stage starts at Hard.
inline constexpr auto EXTRA_LEVEL = GAME_HARD;
inline constexpr auto EXTRA_LIVES = 2;
