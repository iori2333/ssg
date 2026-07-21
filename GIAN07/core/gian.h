///
/// Gian - Global game management
///

#pragma once

// [Change history]

// 2000/02/23 : Achieved performance equivalent to GIAN06
// 2000/02/09 : Major changes

// [Include Files]
#include "constants.h"
#include "config.h"

#include "util/ut_math.h"

// GRP_RES_RECT: native resolution as a rectangle, used by game scenes
inline constexpr WINDOW_LTRB GRP_RES_RECT = {{.x = 0, .y = 0}, GRP_RES};

// [Constants]

// Coordinate related
inline constexpr int X_MIN = 128; // Display X coordinate minimum
inline constexpr int X_MAX = 511; // Display X coordinate maximum
inline constexpr int X_MID = (X_MAX + X_MIN) / 2;
inline constexpr int Y_MIN = 0; // Display Y coordinate minimum
inline constexpr WINDOW_COORD Y_MAX = (GRP_RES.h - 1);
inline constexpr int Y_MID = (Y_MAX + Y_MIN) / 2;

inline constexpr WINDOW_LTRB PLAYFIELD_CLIP = {X_MIN, Y_MIN, (X_MAX + 1),
                                               (Y_MAX + 1)};

inline constexpr int X_RNDV = -30000; // For random X coordinate specification
inline constexpr int Y_RNDV = -30000; // For random Y coordinate specification

inline constexpr int GX_MIN = (X_MIN * 64); // Game coordinate X minimum
inline constexpr int GX_MAX = (X_MAX * 64); // Game coordinate X maximum
inline constexpr int GX_MID = (GX_MAX + GX_MIN) / 2; // Game coordinate X center
inline constexpr int GY_MIN = (Y_MIN * 64); // Game coordinate Y minimum
inline constexpr int GY_MAX = (Y_MAX * 64); // Game coordinate Y maximum
inline constexpr int GY_MID = (GY_MAX + GY_MIN) / 2; // Game coordinate Y center

inline constexpr int SX_WID = (64 * 10);         // Cactus X width?
inline constexpr int SY_WID = (64 * 10);         // Cactus Y width?
inline constexpr int SX_MIN = (GX_MIN + SX_WID); // Cactus X coordinate minimum
inline constexpr int SX_MAX = (GX_MAX - SX_WID); // Cactus X coordinate maximum
inline constexpr int SY_MIN =
    (GY_MIN + SY_WID + (30 * 64));               // Cactus Y coordinate minimum
inline constexpr int SY_MAX = (GY_MAX - SY_WID); // Cactus Y coordinate maximum
inline constexpr int SX_START = GX_MID;          // Cactus start X coordinate
inline constexpr int SY_START =
    (GY_MAX + (180 * 64) /*- 50*64*/); // Cactus start Y coordinate

inline constexpr int RL_WIDX = (32 - 4); // Reflect laser X coordinate offset
inline constexpr int RL_WIDY = 16;       // Reflect laser Y coordinate offset
inline constexpr int RLX_MIN =
    ((GX_MIN / 64) + RL_WIDX); // Reflect laser reflection X minimum
inline constexpr int RLX_MAX =
    ((GX_MAX / 64) - RL_WIDX); // Reflect laser reflection X maximum
inline constexpr int RLY_MIN =
    ((GY_MIN / 64) + RL_WIDY); // Reflect laser reflection Y minimum
inline constexpr int RLY_MAX =
    ((GX_MAX / 64) - RL_WIDY); // Reflect laser reflection Y maximum

inline constexpr int NREG_SX =
    (X_MID - (13 * 9));                       // Name register window start X
inline constexpr int NREG_SY = (Y_MID + 100); // Name register window start Y
inline constexpr int NREGI_X =
    (X_MID - (8 * 7)); // Name register window (name display) start X
inline constexpr int NREGI_Y =
    (Y_MID + 60); // Name register window (name display) start Y

inline constexpr int STG_RNDXY = 0; // Value when placement coordinates are
                                    // random...(not quite sure what it does)

// Score
inline constexpr int SCORE_NAME =
    9; // Max string length for score (including NULL)

// [Functions]
inline short SPEEDM(uint8_t v) {
  return static_cast<short>((v & 0x3f) << 4);
} // For speed setting
inline short WAVESP(uint8_t v) {
  return static_cast<short>(v << 4);
} // For WAVE? speed setting

// X coordinate random (requires runtime rnd(), so not constexpr)
inline int GX_RND() { return (X_MIN + (rnd() % (X_MAX - X_MIN))) << 6; }
// Y coordinate random
inline int GY_RND() { return (Y_MIN + (rnd() % (Y_MAX - Y_MIN))) << 6; }

// Hit check: nonzero if hit
inline bool HITCHK(int a, int b, int h) { return std::abs(a - b) < h; }

void StdStatusOutput(PracticeMode practice_mode);
