///
/// Laser - Laser processing (reflect, short)
///

#pragma once

#include "game/coords.h"
#include "game/graphics_backend.h"
#include <cstdint>

// [Change history]

// 2000/02/17 : Started migration to new system. Completely separated from infinite laser

//-> A bit old from here (1999..)
// (4/3)  10:36 Development started
// (4/6)  12:00 Polygon & clipping function finally completed. When will rendering be done?
// (4/7)  12:02 Decided to handle all lasers with the same structure
// (4/8)   7:23 Infinite laser creation
// (4/9)   2:01 Finished implementing reflect laser
// (4/9)   2:59 Reflect laser complete
// (4/11) 14:05 Short & reflect laser collision detection complete
// (4/11) 15:17 Enhanced reflector hit check (bug fixed but slower)
//
// (9/23) 16:18 Line drawing, ECL support, etc. completed

// Laser constants
inline constexpr auto LASER_MAX = 1000; // Maximum number of lasers

// Laser command structure
struct LaserCommand {
  int x, y; // Starting point coordinates
  int v;    // Laser initial velocity

  int w;  // Laser thickness (uses x64 coordinates)
  int l;  // Laser final length (uses x64 coordinates)
  int l2; // Laser firing position offset (x64...)

  uint8_t d;  // Firing angle
  uint8_t dw; // Firing width

  uint8_t n; // Number of lasers
  uint8_t c; // Laser color

  char a;       // Acceleration (will this be used?)
  uint8_t cmd;  // Laser activation command (mostly same as bullets)
  uint8_t type; // Short, infinite, etc.
  uint8_t notr; // Reflector number that does not reflect
};
// (LASER_CMD alias removed — use LaserCommand directly)

// Laser structure
inline constexpr auto LF_DELETE = 0x80; // Delete laser (remove from processing)

struct LASER_DATA {
  int x, y;   // Current starting point
  int vx, vy; // Velocity (X, Y) components
  int lx, ly; // Display coordinate offset (length)
  int wx, wy; // Display coordinate offset (thickness)
  int v;      // Velocity

  VERTEX_XY p[4]; // Display coordinates

  char a;    // Acceleration (will this be used?)
  uint8_t d; // Direction of travel

  int w, wmax; // Thickness
  int l, lmax; // Current length, target length
  int ltemp;   // Reflect laser variable (used only on fire & hit)

  uint16_t count; // Frame counter
  uint8_t c;      // Color
  uint8_t type;   // Type
  uint8_t flag;   // Deletion request flag etc. (including effects)
  uint8_t notr;   // Reflector number that does not reflect
  uint8_t evade;  // Graze flag
};

// Laser functions
// Backward-compat inline wrapper moved to end of laser_manager.h
// Implementation migrated to LaserManager methods

// Laser various variables
// Access directly via Lasers.cmd, Lasers.count, Lasers.lasers, Lasers.laser_indices
// extern REFLECTOR	Reflector[RT_MAX];		// Reflector structure
// extern uint16_t	ReflectorNow;	// Reflector count
