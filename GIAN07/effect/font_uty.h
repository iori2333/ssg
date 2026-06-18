///
/// FontUty - Font utility functions
///

#pragma once

// [Revision history]

// 2000/07/22 : Partial rewrite for font additions
// 2000/02/19 : Began development of font handling

#include "game/text.h"

// [Functions]
void GrpPut16(int x, int y,
              const char *s); // Draw string in 16x16 transparent font (fast)
void GrpPut16c2(int x, int y,
                const char *s); // Same as above but x-advance is 16
void GrpPutc(int x, int y,
             char c); // Draw char in 16x16 transparent font (w/ clipping)
void GrpPut57(int x, int y, const char *s); // 05x07 opaque font
void GrpPut7B(int x, int y, const char *s); // 07x11 music-room font
void GrpPutScore(int x, int y,
                 const char *s); // Draw score item score

void GrpPutMidNum(int x, int y, int n); // Draw MIDI font

// 5-pixel variable-width font in [SURFACE_ID::SYSTEM]. Supports A-Z.
// ------------------------------------------------------------------

constexpr PIXEL_COORD GrpExtent5(const char c) {
  if ((c < 'A') || (c > 'Z')) {
    assert(!"Character not supported in 5-pixel system font");
    return 0;
  }
  switch (c) {
  case 'I':
    return 3;
    break;
  case 'M':
  case 'T':
  case 'V':
  case 'W':
  case 'Y':
    return 5;
    break;
  default:
    return 4;
    break;
  }
}

constexpr PIXEL_SIZE GrpExtent5(std::string_view s) {
  PIXEL_SIZE ret = {.w = 0, .h = 5};
  for (const char c : s) {
    ret.w += (GrpExtent5(c) + 1);
  }
  ret.w = (std::max)(0, (ret.w - 1));
  return ret;
}

void GrpPut55(WINDOW_POINT topleft, std::string_view s);
// ------------------------------------------------------------------

// Draw gradient font
PIXEL_SIZE DrawGrdFont(TEXTRENDER_SESSION &s, std::span<std::string_view> strs,
                       FONT_ID font, bool shadow,
                       uint8_t (*gradient_func)(PIXEL_COORD y));
