///
/// FontUty - Font utility functions
///

#include "font_uty.h"
#include "loader.h"
#include "platform/graphics_backend.h"
#include "platform/text_backend.h"
#include <format>

#ifdef WIN32
#include <windows.h>

extern constinit const ENUMARRAY<LOGFONTW, FONT_ID> FontSpecs = [] {
  ENUMARRAY<LOGFONTW, FONT_ID> ret;

  LOGFONTW logfont = {.lfEscapement = 0,
                      .lfOrientation = 0,
                      .lfItalic = false,
                      .lfUnderline = false,
                      .lfStrikeOut = false,
                      .lfCharSet = SHIFTJIS_CHARSET,
                      .lfOutPrecision = OUT_TT_ONLY_PRECIS,
                      .lfClipPrecision = CLIP_DEFAULT_PRECIS,
                      .lfQuality = PROOF_QUALITY,
                      .lfPitchAndFamily = FIXED_PITCH,
                      .lfFaceName = L"MS Gothic"};

  // Tiny font
  logfont.lfHeight = 14;
  logfont.lfWidth = 7;
  logfont.lfWeight = FW_NORMAL;
  ret[FONT_ID::SMALL] = logfont;

  // Normal font
  logfont.lfHeight = 16;
  logfont.lfWidth = 8;
  logfont.lfWeight = FW_NORMAL;
  ret[FONT_ID::NORMAL] = logfont;

  // Large font
  logfont.lfHeight = 24;
  logfont.lfWidth = 12;
  logfont.lfWeight = FW_MEDIUM;
  ret[FONT_ID::LARGE] = logfont;

  // Tiny
  logfont.lfHeight = 10;
  logfont.lfWidth = 0;
  logfont.lfWeight = FW_NORMAL;
  ret[FONT_ID::TINY] = logfont;

  return ret;
}();

static constexpr auto FONT_MODERN = L"msgothic.ttc";

void TextBackend_GDIInit() {
  AddFontResourceExW(FONT_MODERN, FR_PRIVATE, nullptr);
}

void TextBackend_GDICleanup() {
  RemoveFontResourceExW(FONT_MODERN, FR_PRIVATE, nullptr);
}
#elifdef LINUX
static constexpr auto GOTHIC = "MS Gothic,IPAMonaGothic ";

extern constinit const ENUMARRAY<const char *, FONT_ID> FontSpecs = {
    (GOTHIC "Regular 14px"),
    (GOTHIC "Regular 16px"),
    (GOTHIC "Medium 24px"),
    (GOTHIC "Regular 10px"),
};

#undef GOTHIC
#endif

// Glyph selection inside the 16x16 font
std::optional<PIXEL_LTRB> Glyph16x16(char c) {
  PIXEL_LTWH src;
  src.w = 16;
  src.h = 16;

  // Character layout in SURFACE_ID::SYSTEM is as follows:
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

// 16x16 transparent font string output (fast)
void GrpPut16(int x, int y, const char *s) {
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 14) {
    auto maybe_src = Glyph16x16(*s);
    if (maybe_src) {
      tx = x;
      ty = y;
      if (tx >= 0 && tx < 630) { // Safety measure???
        GrpSurface_Blit({tx, ty}, SURFACE_ID::SYSTEM, maybe_src.value());
      }
    }
  }
}

// Same as above, but x advance is 16
void GrpPut16c2(int x, int y, const char *s) {
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 16) {
    auto maybe_src = Glyph16x16(*s);
    if (maybe_src) {
      tx = x;
      ty = y;
      // Safety measure???
      GrpSurface_Blit({tx, ty}, SURFACE_ID::SYSTEM, maybe_src.value());
    }
  }
}

// 16x16 transparent font single character output (with clipping)
void GrpPutc(int x, int y, char c) {
  auto maybe_src = Glyph16x16(c);
  if (maybe_src) {
    GrpSurface_Blit({x, y}, SURFACE_ID::SYSTEM, maybe_src.value());
  }
}

// 05x07 solid blit font
void GrpPut57(int x, int y, const char *s) {
  PIXEL_LTRB src;
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 6) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PIXEL_LTWH{(((*s - '0') << 3) + 128), 80, 5, 7};
    } else {
      continue;
    }

    tx = x;
    ty = y;
    if (tx >= 0 && tx < 630) { // Safety measure???
      GrpSurface_Blit({tx, ty}, SURFACE_ID::SYSTEM, src);
    }
  }
}

// 07x11 music room font
void GrpPut7B(int x, int y, const char *s) {
  PIXEL_LTRB src;
  for (; (*s) != '\0'; s++, x += 8) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PIXEL_LTWH{(((*s - '0') << 3) + 184), 456, 7, 11};
    } else if ((*s) == '-') {
      src = PIXEL_LTWH{(264 + 0), 456, 7, 11};
    } else if ((*s) == ':') {
      src = PIXEL_LTWH{(264 + 8), 456, 7, 11};
    } else {
      continue;
    }

    if ((x >= 0) && (x < 630)) { // Safety measure???
      GrpSurface_Blit({x, y}, SURFACE_ID::MUSIC, src);
    }
  }
}

// Draw score item scores
void GrpPutScore(int x, int y, const char *s) {
  PIXEL_LTRB src;
  int sx = 0;
  int tx = 0;
  int ty = 0;

  sx = x;

  for (; (*s) != '\0'; s++, x += 6) {
    if ((*s) >= '0' && (*s) <= '9') {
      src = PIXEL_LTWH{(((*s - '0') << 3) + 128), 88, 5, 7};
    } else {
      continue;
    }

    tx = x;
    ty = y;
    if (tx >= 0 && tx < 630) { // Safety measure???
      GrpSurface_Blit({tx, ty}, SURFACE_ID::SYSTEM, src);
    }
  }
}

void GrpPut55(WINDOW_POINT topleft, std::string_view s) {
  for (const char c : s) {
    const auto w = GrpExtent5(c);
    if (w == 0) {
      continue;
    }
    const auto src = PIXEL_LTWH{(((c - 'A') << 3) + 128), 99, w, 5};
    GrpSurface_Blit(topleft, SURFACE_ID::SYSTEM, src);
    topleft.x += (w + 1);
  }
}

// Draw MIDI font
void GrpPutMidNum(int x, int y, int n) {
  auto buf = std::format("{:3}", n);
  int i = 0;
  PIXEL_LTRB src;

  // n = 1Byte should fit within 3 digits
  for (i = 0; i < 3; i++, x += 5) {
    if (buf[i] >= '0' && buf[i] <= '9') {
      src = PIXEL_LTWH{(80 + ((buf[i] - '0') * 4)), 432, 4, 5};
      GrpSurface_Blit({x, y}, SURFACE_ID::MUSIC, src);
    }
    if (buf[i] == '-') {
      src = PIXEL_LTWH{(80 + (10 * 4)), 432, 4, 5};
      GrpSurface_Blit({x, y}, SURFACE_ID::MUSIC, src);
    }
  }
}

PIXEL_SIZE DrawGrdFont(TEXTRENDER_SESSION &s, std::span<std::string_view> strs,
                       FONT_ID font, bool shadow,
                       uint8_t (*gradient_func)(PIXEL_COORD y)) {
  PIXEL_SIZE extent = {.w = 0, .h = 0};

  // A kind of common trick?
  const auto temp = s.PixelAccess([](TEXTRENDER_SESSION::PIXELACCESS &p) {
    const PIXEL_POINT coord = {.x = 0, .y = 0};
    const auto old = p.GetRaw(coord);
    p.Set(coord, RGB{.r = 255, .g = 255, .b = 255});
    const auto temp = p.GetRaw(coord);
    p.SetRaw(coord, old);
    return temp;
  });

  s.SetFont(font);
  for (const auto &str : strs) {
    if (shadow) {
      s.Put({.x = (extent.w + 2), .y = 2}, str, RGB{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.w + 1), .y = 2}, str, RGB{.r = 0, .g = 0, .b = 128});
      s.Put({.x = (extent.w + 1), .y = 1}, str,
            RGB{.r = 255, .g = 255, .b = 255});
      s.Put({.x = (extent.w + 0), .y = 1}, str,
            RGB{.r = 255, .g = 255, .b = 255});
    } else {
      s.Put({.x = extent.w, .y = 0}, str, RGB{.r = 255, .g = 255, .b = 255});
    }
    extent += s.Extent(str);
  }

  s.PixelAccess([&](TEXTRENDER_SESSION::PIXELACCESS &p) {
    for (PIXEL_COORD y = shadow; y < extent.h; y++) {
      const uint8_t gradient = gradient_func(y);
      const RGB color = {.r = gradient, .g = gradient, .b = 255};
      for (PIXEL_COORD x = shadow; x < extent.w; x++) {
        if (p.GetRaw({.x = x, .y = y}) == temp) { // RGB(255, 255, 255)
          p.Set({.x = x, .y = y}, color);
        }
      }
    }
  });

  return extent;
}
