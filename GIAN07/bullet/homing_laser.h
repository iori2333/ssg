///
/// HomingLaser - Long laser processing
///

#pragma once

#include "core/point.h"

// [Constants]
inline constexpr auto HLASER_MAX = 162;
inline constexpr auto HLASER_LEN = 7;     // Number of draw passes..
inline constexpr auto HLASER_SECTION = 4; // Load width

inline constexpr auto HL_NONE = 0;  // Just move forward
inline constexpr auto HL_TYPE1 = 1; // Type 1

inline constexpr auto HLS_NORM = 0x00;  // Homing laser normal
inline constexpr auto HLS_CLEAR = 0x01; // Homing laser clearing
inline constexpr auto HLS_DEAD = 0xff;  // Homing laser delete request

// [Structs]

// Homing laser
struct HomingLaserData {
  int Current; // Current head index
  int v;       // Speed
  int a;       // Acceleration

  uint32_t Count; // Frame counter

  uint8_t Type;  // Type (acceleration & homing)
  uint8_t State; // State
  uint8_t c;     // Color
  uint8_t Left;  // Remaining homing count

  HomingLaserData *Next;                   // Pointer to next laser
  DegPoint p[HLASER_LEN * HLASER_SECTION]; // Vertex queue (ExDef.h)
};
// (HLaserData alias removed — use HomingLaserData directly)

// Homing laser set info
struct HomingLaserInfo {
  int x, y; // Center coordinates

  uint8_t d;  // Angle
  uint8_t dw; // Angle spread
  uint8_t n;  // Count

  uint8_t c;    // Color
  uint8_t type; // Type
};
// (HLaserInfo alias removed — use HomingLaserInfo directly)

// [Global variables]
// Access directly via Lasers.homing_count, Lasers.homing_cmd

// [Function prototypes]
// Backward-compat inline wrappers moved to end of laser_manager.h
// Implementations moved to LaserManager methods
