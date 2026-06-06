/*
 *   Coordinate conversion helpers for GIAN07 game code
 *
 *   These are simple inline wrappers around the primitives defined in
 *   game/coords.h. They exist to replace raw >> 6 shifts with readable
 *   function calls throughout GIAN07/.
 */

#pragma once

#include "game/coords.h"

// Convert world-space coordinate to pixel-space.
// Replaces the common (x >> 6) and (x >> WORLD_COORD_BITS) patterns.
inline PIXEL_COORD WorldToPixel(WORLD_COORD world) {
  return world >> WORLD_COORD_BITS;
}
