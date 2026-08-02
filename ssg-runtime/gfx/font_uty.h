///
/// FontUty - Font utility functions
///

#pragma once

// [Revision history]

// 2000/07/22 : Partial rewrite for font additions
// 2000/02/19 : Began development of font handling

#include "text.h"

// [Functions]

void DrawFont16(int x, int y,
                const char *s); // Draw string in 16x16 transparent font (fast)
void DrawFont16C2(int x, int y,
                  const char *s); // Same as above but x-advance is 16
void DrawGlyph(int x, int y,
               char c); // Draw char in 16x16 transparent font (w/ clipping)
void DrawFont57(int x, int y, const char *s); // 05x07 opaque font
void DrawFont7B(int x, int y, const char *s); // 07x11 music-room font
void DrawScore(int x, int y,
               const char *s); // Draw score item score

void DrawFontMid(int x, int y, int n); // Draw MIDI font

// 5-pixel variable-width font in [SurfaceId::System]. Supports A-Z.
// ------------------------------------------------------------------

constexpr PixelCoord FontExtent5(const char c) {
  if ((c < 'A') || (c > 'Z')) {
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

constexpr PixelSize FontExtent5(std::string_view s) {
  PixelSize ret = {.w = 0, .h = 5};
  for (const char c : s) {
    ret.w += (FontExtent5(c) + 1);
  }
  ret.w = (std::max)(0, (ret.w - 1));
  return ret;
}

void DrawFont55(WindowPoint topleft, std::string_view s);
// ------------------------------------------------------------------

// Draw gradient font
PixelSize DrawGrdFont(TextRenderSession &s, std::span<std::string_view> strs,
                      FontId font, bool shadow,
                      uint8_t (*gradient_func)(PixelCoord y));
