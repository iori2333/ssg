///
/// Pixel formats
///
#pragma once

#include <cstdint>

// Same as the standard Win32 PALETTEENTRY structure.

struct Rgba {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  constexpr bool operator==(const Rgba &other) const = default;
};

// Same as the standard Win32 RGBQUAD structure.
struct Bgra {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;

  constexpr bool operator==(const Bgra &other) const = default;
};
