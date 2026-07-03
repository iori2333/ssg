///
/// Effect3D - 3D effect processing
///

#pragma once

// [Revision history]
// 2000/05/31 : Development started

// [Header files]
#include <cstdint>
#include <span>

#include "gfx/coords.h"

// [Constants]
inline constexpr auto STG4ROCK_STDMOVE = 0;  // Normal scrolling
inline constexpr auto STG4ROCK_ACCMOVE1 = 1; // Accelerated scrolling (1)
inline constexpr auto STG4ROCK_ACCMOVE2 = 2; // Accelerated scrolling (2)
inline constexpr auto STG4ROCK_3DMOVE = 3;   // 3D rotation
inline constexpr auto STG4ROCK_LEAVE = 4;    // Temporarily erase rocks
inline constexpr auto STG4ROCK_END = 5;      // Effect end

// [Structs]
struct Point3D {
  WORLD_COORD x, y, z;
};

struct LineList3D {
  PIXEL_POINT center;       // Vertex coordinate correction
  std::span<WORLD_POINT> p; // Vertex coordinates

  double DegX, DegY, DegZ; // Rotation angle for each axis (radians)
};

struct Circle3D {
  int ox, oy;
  int r;
  uint8_t deg;
  uint8_t n;
};

struct Deg3D {
  double dx;
  double dy;
  double dz;
};

struct Cube3D {
  Point3D p;
  Deg3D d;
  int l;
};

struct Star2D {
  int x, y;
  int vy;
};

// Cloud management struct
struct Cloud2D {
  int x, y;     // x64 coordinate
  int vy;       // Velocity y-component (only y)
  uint8_t type; // Cloud type
};

// Non-general-purpose 2D square wireframe
struct WFLine2D {
  int ox, oy; // Center coordinates
  int w;      // Square side length
  uint8_t d;  // Square tilt angle
};

// Fake ECL enumeration management struct
struct FakeECLString {
  int SrcX, SrcY; // Source image reference X and Y coordinates
  int x, y;       // Current coordinates x64
  int vx, vy;     // Current velocity components x64
};

// Rock management struct
struct Rock3D {
  int x, y, z; // Current coordinates
  int vx, vy;  // Velocity XY components (on 2D plane)

  uint32_t count; // Counter
  int v;          // Velocity

  char a;        // Acceleration
  uint8_t d;     // Angle (on 2D plane)
  uint8_t GrpID; // Graphics ID
  uint8_t State; // Current state
};

// [Functions]
// Utility functions (not manager methods)
void InitLineList3D(std::span<LineList3D> w);
void DrawLineList3D(std::span<const LineList3D> w);
void MoveWarningR(char count);

// Backward compat inline wrapper moved to end of effect_manager.h
// Implementation moved to EffectManager methods
