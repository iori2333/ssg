///
/// Text rendering via GDI
///

#include <vector>

#include <windows.h>

#include "surface_gdi.h"

#include "platform/text_backend.h"
#include "util/enum_array.h"

extern const util::EnumArray<LOGFONTW, FONT_ID> FontSpecs;

template <typename F> auto WithWideUTF8(std::string_view str, F &&func) {
  const int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(),
                                      (int)str.size(), nullptr, 0);
  if (len <= 0) {
    return decltype(func(std::wstring_view{})){};
  }
  std::vector<wchar_t> buf(len);
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data(),
                      (int)str.size(), buf.data(), len);
  return func(std::wstring_view{buf.data(), buf.size()});
}

TEXTRENDER TextObj;

static class {
  util::EnumArray<HFONT, FONT_ID> arr;

public:
  HFONT ForID(FONT_ID font) {
    if (!arr[font]) {
      if (std::ranges::all_of(arr, [](auto h) { return !h; })) {
        TextBackend_GDIInit();
      }
      arr[font] = CreateFontIndirectW(&FontSpecs[font]);
    }
    return arr[font];
  }

  void Cleanup(void) {
    for (auto &font : arr) {
      if (font) {
        DeleteObject(font);
        font = nullptr;
      }
    }
  }
} Fonts;

PIXEL_SIZE TextGDIExtent(std::optional<HFONT> font, std::string_view str) {
  const auto hdc = GrpSurface_GDIText_Surface().dc;
  const auto font_prev = (font ? SelectObject(hdc, font.value()) : nullptr);
  const auto ret = WithWideUTF8(str, [&](const std::wstring_view str_w) {
    SIZE ret = {0, 0};
    if (!GetTextExtentPoint32W(hdc, str_w.data(), str_w.size(), &ret)) {
      return PIXEL_SIZE{0, 0};
    }
    return PIXEL_SIZE{ret.cx, ret.cy};
  });
  if (font && font_prev) {
    SelectObject(hdc, font_prev);
  }
  return ret;
}

uint32_t TEXTRENDER_SESSION::PIXELACCESS::GetRaw(const PIXEL_POINT &xy_rel) {
  const auto hdc = GrpSurface_GDIText_Surface().dc;
  return GetPixel(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y));
}

void TEXTRENDER_SESSION::PIXELACCESS::SetRaw(const PIXEL_POINT &xy_rel,
                                             uint32_t color) {
  const auto hdc = GrpSurface_GDIText_Surface().dc;
  SetPixelV(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y), color);
}

RGB TEXTRENDER_SESSION::PIXELACCESS::Get(const PIXEL_POINT &xy_rel) {
  const auto ret = GetRaw(xy_rel);
  return RGB{.r = GetRValue(ret), .g = GetGValue(ret), .b = GetBValue(ret)};
}

void TEXTRENDER_SESSION::PIXELACCESS::Set(const PIXEL_POINT &xy_rel,
                                          const RGB color) {
  SetRaw(xy_rel, RGB(color.r, color.g, color.b));
}

TEXTRENDER_SESSION::TEXTRENDER_SESSION(const PIXEL_LTWH &rect) : rect(rect) {
  const auto hdc = GrpSurface_GDIText_Surface().dc;

  // Clear the rectangle, for two reasons:
  // • The caller is free to render multiple transparent strings on top
  //   of each other, so we can't rely on SetBkMode(hdc, OPAQUE).
  // • The next text might be shorter than the previous one in this
  //   rectangle.
  const PIXEL_LTRB ltrb = rect;
  const RECT r = {ltrb.left, ltrb.top, ltrb.right, ltrb.bottom};
  auto black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  const auto rect_filled_with_black = FillRect(hdc, &r, black);
  const auto background_mode_set_to_transparent = SetBkMode(hdc, TRANSPARENT);
  assert(rect_filled_with_black);
  assert(background_mode_set_to_transparent);
}

TEXTRENDER_SESSION::~TEXTRENDER_SESSION() {
  if (font_initial) {
    const auto hdc = GrpSurface_GDIText_Surface().dc;
    SelectObject(hdc, font_initial.value());
  }
  GrpSurface_GDIText_Update(rect);
}

PIXEL_SIZE TEXTRENDER_SESSION::RectSize(void) const { return {rect.w, rect.h}; }

void TEXTRENDER_SESSION::SetFont(FONT_ID font) {
  if (font_cur != font) {
    const auto hdc = GrpSurface_GDIText_Surface().dc;
    auto font_prev = SelectObject(hdc, Fonts.ForID(font));
    if (!font_initial) {
      font_initial = font_prev;
    }
    font_cur = font;
  }
}

void TEXTRENDER_SESSION::SetColor(const RGB color) {
  const COLORREF color_gdi = RGB(color.r, color.g, color.b);
  if (color_cur != color_gdi) {
    const auto hdc = GrpSurface_GDIText_Surface().dc;
    SetTextColor(hdc, color_gdi);
    color_cur = color_gdi;
  }
}

PIXEL_SIZE TEXTRENDER_SESSION::Extent(std::string_view str) {
  return TextGDIExtent(std::nullopt, str);
}

void TEXTRENDER_SESSION::Put(const PIXEL_POINT &topleft_rel,
                             std::string_view str, std::optional<RGB> color) {
  WithWideUTF8(str, [&](const std::wstring_view str_w) {
    const auto hdc = GrpSurface_GDIText_Surface().dc;
    if (color) {
      SetColor(color.value());
    }
    const RECT r = {
        .left = (rect.left + topleft_rel.x),
        .top = (rect.top + topleft_rel.y),
        .right = (rect.left + rect.w),
        .bottom = (rect.top + rect.h),
    };
    return ExtTextOutW(hdc, r.left, r.top, ETO_CLIPPED, &r, str_w.data(),
                       str_w.size(), nullptr);
  });
}

bool TEXTRENDER::Wipe() {
  if (!bounds || (bounds.w > (std::numeric_limits<int32_t>::max)()) ||
      (bounds.h > (std::numeric_limits<int32_t>::max)())) {
    assert(!"Invalid size for blank surface");
    return false;
  }
  const auto w = static_cast<int32_t>(bounds.w);
  const auto h = static_cast<int32_t>(bounds.h);
  return (GrpSurface_GDIText_Create(w, h, {0x00, 0x00, 0x00}) &&
          TEXTRENDER_PACKED::Wipe());
}

std::optional<TEXTRENDER_SESSION>
TEXTRENDER::Session(TEXTRENDER_RECT_ID rect_id) {
  const auto &surf = GrpSurface_GDIText_Surface();
  if ((surf.size != bounds) && !Wipe()) {
    return std::nullopt;
  }
  assert(rect_id < rects.size());
  return TEXTRENDER_SESSION{rects[rect_id].rect};
}

void TEXTRENDER::WipeBeforeNextRender() {
  TEXTRENDER_PACKED::Wipe();

  // This also skips the needless creation of an uninitialized surface
  // during DirectDraw's init function.
  GrpSurface_GDIText_Surface().size = {0, 0};
}

PIXEL_SIZE TEXTRENDER::TextExtent(FONT_ID font, std::string_view str) {
  return TextGDIExtent(Fonts.ForID(font), str);
}

void TextBackend_Cleanup(void) {
  Fonts.Cleanup();
  TextBackend_GDICleanup();
}
