/// Bitmap fonts embedded in the game's image atlases.

#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string_view>

#include "bitmap_font.h"

#include "gfx/graphics.h"
#include "gfx/text/text_renderer.h"

namespace ui {

// Glyph selection inside the 16x16 font
namespace {
std::optional<Rect> Glyph16(char c) {
  PixelPoint origin;

  // Character layout in SurfaceId::System is as follows:
  // (subject to change, so be careful)
  // ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
  // abcdefghijklmnopqrstuvwxyz!?#\<>=,+-

  if ((c >= 'A') && (c <= 'Z')) {
    origin.x = ((c - 'A') << 4);
    origin.y = (480 - 32);
  } else if ((c >= 'a') && (c <= 'z')) {
    origin.x = ((c - 'a') << 4);
    origin.y = (480 - 16);
  } else if ((c >= '0') && (c <= '9')) {
    origin.x = (((c - '0') << 4) + 416);
    origin.y = (480 - 32);
  } else {
    switch (c) {
    case '!':
      origin.x = ((0 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '?':
      origin.x = ((1 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '#':
      origin.x = ((2 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '\\':
      origin.x = ((3 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '<':
      origin.x = ((4 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '>':
      origin.x = ((5 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '=':
      origin.x = ((6 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case ',':
      origin.x = ((7 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '+':
      origin.x = ((8 << 4) + 416);
      origin.y = (480 - 16);
      break;
    case '-':
      origin.x = ((9 << 4) + 416);
      origin.y = (480 - 16);
      break;
    default:
      return std::nullopt;
    }
  }
  return Rect::FromLtwh(origin.x, origin.y, 16, 16);
}
} // namespace

void Draw16(PixelPoint topleft, std::string_view text, int advance) {
  for (const char glyph : text) {
    auto maybe_src = Glyph16(glyph);
    if (maybe_src) {
      GraphicsSurfaceBlit(topleft, SurfaceId::System, *maybe_src);
    }
    topleft.x += advance;
  }
}

void DrawGlyph(PixelPoint topleft, char glyph) {
  auto maybe_src = Glyph16(glyph);
  if (maybe_src) {
    GraphicsSurfaceBlit(topleft, SurfaceId::System, *maybe_src);
  }
}

void DrawDigits5x7(PixelPoint topleft, std::string_view text) {
  Rect src;
  for (const char glyph : text) {
    if (glyph >= '0' && glyph <= '9') {
      src = Rect::FromLtwh((((glyph - '0') << 3) + 128), 80, 5, 7);
    } else {
      topleft.x += 6;
      continue;
    }
    GraphicsSurfaceBlit(topleft, SurfaceId::System, src);
    topleft.x += 6;
  }
}

void DrawMusicDigits(PixelPoint topleft, std::string_view text) {
  Rect src;
  for (const char glyph : text) {
    if (glyph >= '0' && glyph <= '9') {
      src = Rect::FromLtwh((((glyph - '0') << 3) + 184), 456, 7, 11);
    } else if (glyph == '-') {
      src = Rect::FromLtwh((264 + 0), 456, 7, 11);
    } else if (glyph == ':') {
      src = Rect::FromLtwh((264 + 8), 456, 7, 11);
    } else {
      topleft.x += 8;
      continue;
    }
    GraphicsSurfaceBlit(topleft, SurfaceId::Music, src);
    topleft.x += 8;
  }
}

void DrawScore(PixelPoint topleft, std::string_view text) {
  Rect src;
  for (const char glyph : text) {
    if (glyph >= '0' && glyph <= '9') {
      src = Rect::FromLtwh((((glyph - '0') << 3) + 128), 88, 5, 7);
    } else {
      topleft.x += 6;
      continue;
    }
    GraphicsSurfaceBlit(topleft, SurfaceId::System, src);
    topleft.x += 6;
  }
}

void DrawTinyUpper(PixelPoint topleft, std::string_view text) {
  for (const char c : text) {
    const auto w = TinyGlyphWidth(c);
    if (w == 0) {
      continue;
    }
    const auto src = Rect::FromLtwh((((c - 'A') << 3) + 128), 99, w, 5);
    GraphicsSurfaceBlit(topleft, SurfaceId::System, src);
    topleft.x += (w + 1);
  }
}

void DrawMidiValue(PixelPoint topleft, int value) {
  const auto text = std::format("{:3}", value);
  Rect src;

  for (const char glyph : text) {
    if (glyph >= '0' && glyph <= '9') {
      src = Rect::FromLtwh((80 + ((glyph - '0') * 4)), 432, 4, 5);
      GraphicsSurfaceBlit(topleft, SurfaceId::Music, src);
    }
    if (glyph == '-') {
      src = Rect::FromLtwh((80 + (10 * 4)), 432, 4, 5);
      GraphicsSurfaceBlit(topleft, SurfaceId::Music, src);
    }
    topleft.x += 5;
  }
}

PixelPoint DrawGradient(TextRenderSession &s,
                        std::span<std::string_view> strings, FontId font,
                        bool shadow, uint8_t (*gradient_func)(int y)) {
  PixelPoint extent = {.x = 0, .y = 0};

  // A kind of common trick?
  const auto temp = s.EditPixels([](TextRenderSession::PixelSession &p) {
    const PixelPoint coord = {.x = 0, .y = 0};
    const auto old = p.GetRaw(coord);
    p.Set(coord, Rgb{.r = 255, .g = 255, .b = 255});
    const auto temp = p.GetRaw(coord);
    p.SetRaw(coord, old);
    return temp;
  });

  s.SetFont(font);
  for (const auto &str : strings) {
    if (shadow) {
      s.Put({.x = (extent.x + 2), .y = 2}, str, Rgb{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.x + 1), .y = 2}, str, Rgb{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.x + 1), .y = 1}, str,
            Rgb{.r = 255, .g = 255, .b = 255});
      s.Put({.x = (extent.x + 0), .y = 1}, str,
            Rgb{.r = 255, .g = 255, .b = 255});
    } else {
      s.Put({.x = extent.x, .y = 0}, str, Rgb{.r = 255, .g = 255, .b = 255});
    }
    extent += s.Extent(str);
  }

  s.EditPixels([&](TextRenderSession::PixelSession &p) {
    const auto area = s.RectSize();
    const auto width = std::min(extent.x, area.x);
    const auto height = std::min(extent.y, area.y);
    for (int y = shadow; y < height; y++) {
      const uint8_t gradient = gradient_func(y);
      const Rgb color = {.r = gradient, .g = gradient, .b = 255};
      for (int x = shadow; x < width; x++) {
        if (p.GetRaw({.x = x, .y = y}) == temp) { // Rgb(255, 255, 255)
          p.Set({.x = x, .y = y}, color);
        }
      }
    }
  });

  return extent;
}

} // namespace ui
