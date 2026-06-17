/*
 *   難易度 ＆ オプション設定
 *
 */

#pragma once

#include <cstdint>

inline constexpr uint8_t GAME_EASY = 0;    // 難易度：Ｅａｓｙ
inline constexpr uint8_t GAME_NORMAL = 1;  // 難易度：Ｎｏｒｍａｌ
inline constexpr uint8_t GAME_HARD = 2;    // 難易度：Ｈａｒｄ
inline constexpr uint8_t GAME_LUNATIC = 3; // 難易度：Ｌｕｎａｔｉｃ
inline constexpr uint8_t GAME_EXTRA = 4;   // Ｅｘｔｒａ時...

// Extra Stage starts at Hard.
inline constexpr auto EXTRA_LEVEL = GAME_HARD;
inline constexpr auto EXTRA_LIVES = 2;
