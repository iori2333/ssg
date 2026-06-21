///
/// LongLaser - Long laser processing
///

#pragma once

// Revision history
// 2000/05/29 : Bug fix for 8-bit mode rendering
// 2000/03/22 : Laser function ID changed from laser array ID to the index of
//            : the laser being fired by that enemy

#include "enemy/enemy.h"
#include "gfx/graphics_backend.h"

// Laser constants 2
inline constexpr auto LLASER_MAX = 20;
inline constexpr auto LLASER_EVADE = 1; // Laser graze threshold

// Laser type constants 2
inline constexpr auto LLS_LONG = 0x00;
inline constexpr auto LLS_LONGY = 0x01;
inline constexpr auto LLS_SETDEG = 0x02;
inline constexpr auto LLS_LONGZ = 0x03; // Player set

// Laser flags 2
inline constexpr auto LLF_DISABLE = 0x00; // Laser not in use
inline constexpr auto LLF_NORM = 0x01;    // Laser fully opened
inline constexpr auto LLF_OPEN = 0x02;    // Laser opening
inline constexpr auto LLF_CLOSE = 0x04;   // Laser closing
inline constexpr auto LLF_CLOSEL = 0x08;  // Laser to line state
inline constexpr auto LLF_LINE = 0x10;    // Laser is in line state

// Laser command struct 2
struct LongLaserCommand {
  EnemyData *e; // Pointer to enemy data

  int dx, dy; // Laser launch coordinate offset
  int v;      // Laser speed

  int w; // Laser final thickness

  uint8_t d; // Laser launch angle

  uint8_t c;    // Laser color
  uint8_t type; // Laser type
};
// (LLASER_CMD alias removed — use LongLaserCommand directly)

// Laser struct 2
struct LongLaserData {
  EnemyData *e; // Pointer to enemy data (boss or minion can fire)

  int x, y;       // Current display coordinates
  int dx, dy;     // Offset from enemy data (x64)
  int lx, ly;     // Vector to laser circle center (Grp)
  int infx, infy; // Temporary vector to infinity (Grp)
  int wx, wy;     // Laser width (Grp)

  int w, wmax; // Width, max width (x64)
  int v;

  uint32_t count; // Frame counter

  VERTEX_XY p[4]; // Coordinate storage pointer (Grp)

  uint8_t d; // Laser launch angle
  uint8_t c; // Laser color

  uint8_t flag;    // Laser state
  uint8_t type;    // Laser type
  uint8_t EnemyID; // Enemy-relative index
};
// (LLASER_DATA alias removed — use LongLaserData directly)

// Laser functions 2
// Backward compat inline wrapper moved to end of laser_manager.h
// Implementation moved to LaserManager methods

// Laser variables 2
// Access directly via Lasers.long_lasers, Lasers.long_cmd
