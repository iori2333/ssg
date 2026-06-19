///
/// PlayerShot - Maid shot processing
///

#pragma once

#include "bullet/bullet.h"

// [ Constants ]

// Maximum
inline constexpr auto MAIDTAMA_MAX = 200; // Maximum number of player shots

inline constexpr auto TID_WIDE_MAIN = 0x00; // Wide main shot ID
inline constexpr auto TID_WIDE_SUB = 0x01;  // Wide sub shot ID
inline constexpr auto TID_HOMING_MAIN =
    0x02;                                    // Homing main shot ID
inline constexpr auto TID_HOMING_SUB = 0x03; // Homing sub shot ID
inline constexpr auto TID_LASER_MAIN =
    0x04; // Laser main shot ID
inline constexpr auto TID_LASER_SUB = 0x05; // Laser sub shot ID

inline constexpr auto TID_HOMING_BOMB_A = 0x06; // Homing bomb (moving)
inline constexpr auto TID_HOMING_BOMB_B = 0x07; // Homing bomb (chain explosion)

// Focus (low-speed) form shot IDs. Laser focus reuses TID_LASER_SUB.
inline constexpr auto TID_WIDE_FOCUS_MAIN = 0x08;   // Wide focus main shot ID
inline constexpr auto TID_WIDE_FOCUS_SUB = 0x09;    // Wide focus sub shot ID
inline constexpr auto TID_HOMING_FOCUS_MAIN = 0x0a; // Homing focus main shot ID
inline constexpr auto TID_HOMING_FOCUS_SUB = 0x0b;  // Homing focus sub shot ID

inline constexpr auto TDM_WIDE_MAIN = 6; // Wide main shot damage
inline constexpr auto TDM_WIDE_SUB = 4;  // Wide sub shot damage
inline constexpr auto TDM_HOMING_MAIN =
    6; // Homing main shot damage
inline constexpr auto TDM_HOMING_SUB = 7; // Homing sub shot damage
inline constexpr auto TDM_LASER_MAIN = 2; // Laser main shot damage
inline constexpr auto TDM_LASER_SUB = 5;  // Laser sub shot damage

// Focus (low-speed) form damages. WIDE/LASER/HOMING focus gain no per-hit
// bonus over the base form; their edge is shot pattern (straight columns /
// tighter grouping), not raw damage.
inline constexpr auto TDM_WIDE_FOCUS_MAIN = 6;  // Wide focus main shot damage
inline constexpr auto TDM_WIDE_FOCUS_SUB = 4;   // Wide focus sub shot damage
inline constexpr auto TDM_HOMING_FOCUS_MAIN = 6; // Homing focus main shot damage
inline constexpr auto TDM_HOMING_FOCUS_SUB = 7; // Homing focus sub shot damage

// [ Functions ]

// [ Variables ]
// Access via Players.maid_tama, Players.maid_tama_ind, Players.maid_tama_now
