///
/// FontUty - Font utility functions
///

#include <cstdint>
#include <format>
#include <optional>
#include <span>
#include <string_view>

#include "font_uty.h"

#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "platform/windows/text_gdi.h"

#ifdef WIN32

// NOLINTBEGIN(misc-include-cleaner) - Windows SDK headers require windows.h.
#include <windows.h>

static constexpr auto kModernFont = L"msgothic.ttc";

void TextBackendGDIInit() {
  AddFontResourceExW(kModernFont, FR_PRIVATE, nullptr);
}

void TextBackendGDICleanup() {
  RemoveFontResourceExW(kModernFont, FR_PRIVATE, nullptr);
}
// NOLINTEND(misc-include-cleaner)
#endif

// Glyph selection inside the 16x16 font
namespace {
std::optional<PixelLtrb> Glyph16(char c) {
  PixelLtwh src;
  src.w = 16;
  src.h = 16;

  // Character layout in SurfaceId::System is as follows:
  // (subject to change, so be careful)
  // ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
  // abcdefghijklmnopqrstuvwxyz!?#\<>=,+-

  if ((c >= 'A') && (c <= 'Z')) {
    src.left = ((c - 'A') << 4);
    src.top = (480 - 32);
  } else if ((c >= 'a') && (c <= 'z')) {
    src.left = ((c - 'a') << 4);
    src.top = (480 - 16);
  } else if ((c >= '0') && (c <= '9')) {
    src.left = (((c - '0') << 4) + 416);
    src.top = (480 - 32);
  } else {
    switch (c) {
    case '!':
      src.left = ((0 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '?':
      src.left = ((1 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '#':
      src.left = ((2 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '\\':
      src.left = ((3 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '<':
      src.left = ((4 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '>':
      src.left = ((5 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '=':
      src.left = ((6 << 4) + 416);
      src.top = (480 - 16);
      break;
    case ',':
      src.left = ((7 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '+':
      src.left = ((8 << 4) + 416);
      src.top = (480 - 16);
      break;
    case '-':
      src.left = ((9 << 4) + 416);
      src.top = (480 - 16);
      break;
    default:
      return std::nullopt;
    }
  }
  return src;
}
} // namespace

// 16x16 transparent font string output (fast)
void DrawFont16(int x, int y, const char *s) {
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 14) {
    auto maybe_src = Glyph16(*s);
    if (maybe_src) {
      tx = x;
      ty = y;
      if (tx >= 0 && tx < 630) { // Safety measure???
        GraphicsSurfaceBlit({tx, ty}, SurfaceId::System, maybe_src.value());
      }
    }
  }
}

// Same as above, but x advance is 16
void DrawFont16C2(int x, int y, const char *s) {
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 16) {
    auto maybe_src = Glyph16(*s);
    if (maybe_src) {
      tx = x;
      ty = y;
      // Safety measure???
      GraphicsSurfaceBlit({tx, ty}, SurfaceId::System, maybe_src.value());
    }
  }
}

// 16x16 transparent font single character output (with clipping)
void DrawGlyph(int x, int y, char c) {
  auto maybe_src = Glyph16(c);
  if (maybe_src) {
    GraphicsSurfaceBlit({x, y}, SurfaceId::System, maybe_src.value());
  }
}

// 05x07 solid blit font
void DrawFont57(int x, int y, const char *s) {
  PixelLtrb src;
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 6) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PixelLtwh{(((*s - '0') << 3) + 128), 80, 5, 7};
    } else {
      continue;
    }

    tx = x;
    ty = y;
    if (tx >= 0 && tx < 630) { // Safety measure???
      GraphicsSurfaceBlit({tx, ty}, SurfaceId::System, src);
    }
  }
}

// 07x11 music room font
void DrawFont7B(int x, int y, const char *s) {
  PixelLtrb src;
  for (; (*s) != '\0'; s++, x += 8) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PixelLtwh{(((*s - '0') << 3) + 184), 456, 7, 11};
    } else if ((*s) == '-') {
      src = PixelLtwh{(264 + 0), 456, 7, 11};
    } else if ((*s) == ':') {
      src = PixelLtwh{(264 + 8), 456, 7, 11};
    } else {
      continue;
    }

    if ((x >= 0) && (x < 630)) { // Safety measure???
      GraphicsSurfaceBlit({x, y}, SurfaceId::Music, src);
    }
  }
}

// Draw score item scores
void DrawScore(int x, int y, const char *s) {
  PixelLtrb src;
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 6) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PixelLtwh{(((*s - '0') << 3) + 128), 88, 5, 7};
    } else {
      continue;
    }

    tx = x;
    ty = y;
    if (tx >= 0 && tx < 630) { // Safety measure???
      GraphicsSurfaceBlit({tx, ty}, SurfaceId::System, src);
    }
  }
}

void DrawFont55(WindowPoint topleft, std::string_view s) {
  for (const char c : s) {
    const auto w = FontExtent5(c);
    if (w == 0) {
      continue;
    }
    const auto src = PixelLtwh{(((c - 'A') << 3) + 128), 99, w, 5};
    GraphicsSurfaceBlit(topleft, SurfaceId::System, src);
    topleft.x += (w + 1);
  }
}

// Draw MIDI font
void DrawFontMid(int x, int y, int n) {
  auto buf = std::format("{:3}", n);
  int i = 0;
  PixelLtrb src;

  // n = 1Byte should fit within 3 digits
  for (i = 0; i < 3; i++, x += 5) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      src = PixelLtwh{(80 + ((buf[i] - '0') * 4)), 432, 4, 5};
      GraphicsSurfaceBlit({x, y}, SurfaceId::Music, src);
    }
    if (buf[i] == '-') {
      src = PixelLtwh{(80 + (10 * 4)), 432, 4, 5};
      GraphicsSurfaceBlit({x, y}, SurfaceId::Music, src);
    }
  }
}

PixelSize DrawGrdFont(TextRenderSession &s, std::span<std::string_view> strs,
                      FontId font, bool shadow,
                      uint8_t (*gradient_func)(PixelCoord y)) {
  PixelSize extent = {.w = 0, .h = 0};

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
  for (const auto &str : strs) {
    if (shadow) {
      s.Put({.x = (extent.w + 2), .y = 2}, str, Rgb{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.w + 1), .y = 2}, str, Rgb{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.w + 1), .y = 1}, str,
            Rgb{.r = 255, .g = 255, .b = 255});
      s.Put({.x = (extent.w + 0), .y = 1}, str,
            Rgb{.r = 255, .g = 255, .b = 255});
    } else {
      s.Put({.x = extent.w, .y = 0}, str, Rgb{.r = 255, .g = 255, .b = 255});
    }
    extent += TextRenderSession::Extent(str);
  }

  s.EditPixels([&](TextRenderSession::PixelSession &p) {
    for (PixelCoord y = shadow; y < extent.h; y++) {
      const uint8_t gradient = gradient_func(y);
      const Rgb color = {.r = gradient, .g = gradient, .b = 255};
      for (PixelCoord x = shadow; x < extent.w; x++) {
        if (p.GetRaw({.x = x, .y = y}) == temp) { // Rgb(255, 255, 255)
          p.Set({.x = x, .y = y}, color);
        }
      }
    }
  });

  return extent;
}
