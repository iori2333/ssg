///
/// Lens - Lens effect
///

#pragma once

#include "gfx/coords.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

// [Struct]

// Lens data definition structure
struct LensInfo {
  uint16_t r;                       // Lens radius
  uint16_t Height;                  // Lens diameter
  std::unique_ptr<uint32_t[]> Data; // Lens replacement table

  // Per-frame capture of the original back-buffer pixels under the lens.
  std::unique_ptr<std::byte[]> FOV;

  // GrpLock() functions: draw the lens ball
  void Draw(WINDOW_POINT center);
};

// [Functions]

// Create a lens with radius:r and bulge:m
std::optional<LensInfo> GrpCreateLensBall(uint16_t r, uint16_t m);
