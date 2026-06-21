///
/// Pixel formats
///
#pragma once

#include <cstdint>

// Same as the standard Win32 PALETTEENTRY structure.
struct RGBA {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  constexpr bool operator==(const RGBA &other) const = default;
};
static_assert(sizeof(RGBA) == 4);

// Same as the standard Win32 RGBQUAD structure.
struct BGRA {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;

  constexpr bool operator==(const BGRA &other) const = default;
};
static_assert(sizeof(BGRA) == 4);
