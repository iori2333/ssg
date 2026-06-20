///
/// Pixel formats
///
#pragma once

#include <cstdint>
#include <utility>

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

struct PIXELFORMAT {
  // All specific formats are in memory byte order. The alpha channel must
  // always be valid for formats with an `A` (i.e., 0xFF everywhere for fully
  // opaque images like render backbuffers or the like), and always be
  // ignored for formats with a `X`.
  enum FORMAT {
    PALETTE8,
    RGB565_LE16,   // GGGBBBBB RRRRRGGG
    ARGB1555_LE16, // GGGBBBBB ARRRRRGG
    BGRX8888,
    BGRA8888,
    RGBA8888,
  } format;

  enum SIZE {
    SIZE8 = 1,
    SIZE16 = 2,
    SIZE32 = 4,
  };

  bool IsPalettized() const { return (format == PALETTE8); }

  bool IsChanneled() const { return !IsPalettized(); }

  SIZE PixelSize() const {
    switch (format) {
    case PALETTE8:
      return SIZE8;
    case RGB565_LE16:
    case ARGB1555_LE16:
      return SIZE16;
    case BGRX8888:
    case BGRA8888:
    case RGBA8888:
      return SIZE32;
    }
    std::unreachable();
  }

  size_t PixelByteSize() const { return static_cast<size_t>(PixelSize()); }
};

