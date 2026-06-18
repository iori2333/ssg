///
/// Point - Miscellaneous definitions
///

#pragma once

#include <cstdint>

// Structure for managing angular coordinates
struct DegPoint {
  int x, y;  // Coordinates
  uint8_t d; // Angle
};
