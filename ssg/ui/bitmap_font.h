/// Bitmap fonts embedded in the game's image atlases.
#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

#include "gfx/core/constants.h"
#include "gfx/core/coords.h"

class TextRenderSession;

namespace ui {

void Draw16(PixelPoint topleft, std::string_view text, int advance = 14);
void DrawGlyph(PixelPoint topleft, char glyph);
void DrawDigits5x7(PixelPoint topleft, std::string_view text);
void DrawMusicDigits(PixelPoint topleft, std::string_view text);
void DrawScore(PixelPoint topleft, std::string_view text);
void DrawMidiValue(PixelPoint topleft, int value);

constexpr int TinyGlyphWidth(const char glyph) {
  if (glyph < 'A' || glyph > 'Z') {
    return 0;
  }
  switch (glyph) {
  case 'I':
    return 3;
  case 'M':
  case 'T':
  case 'V':
  case 'W':
  case 'Y':
    return 5;
  default:
    return 4;
  }
}

constexpr PixelPoint TinyExtent(std::string_view text) {
  PixelPoint result = {.x = 0, .y = 5};
  for (const char glyph : text) {
    result.x += TinyGlyphWidth(glyph) + 1;
  }
  result.x = std::max(0, result.x - 1);
  return result;
}

void DrawTinyUpper(PixelPoint topleft, std::string_view text);

PixelPoint DrawGradient(TextRenderSession &session,
                        std::span<std::string_view> strings, FontId font,
                        bool shadow, uint8_t (*gradient_func)(int y));

} // namespace ui
