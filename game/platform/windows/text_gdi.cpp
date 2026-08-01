///
/// Text rendering via GDI
///

#include <vector>

#include <windows.h>

#include "surface_gdi.h"

#include "platform/text_backend.h"
#include "util/enum_array.h"

namespace {

constinit const util::EnumArray<LOGFONTW, FontId> kFontSpecs = [] {
  util::EnumArray<LOGFONTW, FontId> specs;

  LOGFONTW font = {.lfEscapement = 0,
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

  font.lfHeight = 14;
  font.lfWidth = 7;
  font.lfWeight = FW_NORMAL;
  specs[FontId::Small] = font;

  font.lfHeight = 16;
  font.lfWidth = 8;
  specs[FontId::Normal] = font;

  font.lfHeight = 24;
  font.lfWidth = 12;
  font.lfWeight = FW_MEDIUM;
  specs[FontId::Large] = font;

  font.lfHeight = 10;
  font.lfWidth = 0;
  font.lfWeight = FW_NORMAL;
  specs[FontId::Tiny] = font;

  return specs;
}();

} // namespace

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

TextRender &TextRenderer() {
  static TextRender renderer;
  return renderer;
}

namespace {

class FontCache {
  util::EnumArray<HFONT, FontId> arr;

public:
  HFONT ForID(FontId font) {
    if (!arr[font]) {
      if (std::ranges::all_of(arr, [](auto h) { return !h; })) {
        TextBackendGDIInit();
      }
      arr[font] = CreateFontIndirectW(&kFontSpecs[font]);
    }
    return arr[font];
  }

  void Cleanup() {
    for (auto &font : arr) {
      if (font) {
        DeleteObject(font);
        font = nullptr;
      }
    }
  }
};

FontCache &Fonts() {
  static FontCache fonts;
  return fonts;
}

} // namespace

PixelSize TextGDIExtent(std::optional<HFONT> font, std::string_view str) {
  const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
  const auto font_prev = (font ? SelectObject(hdc, font.value()) : nullptr);
  const auto ret = WithWideUTF8(str, [&](const std::wstring_view str_w) {
    SIZE ret = {0, 0};
    if (!GetTextExtentPoint32W(hdc, str_w.data(), str_w.size(), &ret)) {
      return PixelSize{0, 0};
    }
    return PixelSize{ret.cx, ret.cy};
  });
  if (font && font_prev) {
    SelectObject(hdc, font_prev);
  }
  return ret;
}

uint32_t TextRenderSession::PixelSession::GetRaw(const PixelPoint &xy_rel) {
  const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
  return GetPixel(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y));
}

void TextRenderSession::PixelSession::SetRaw(const PixelPoint &xy_rel,
                                             uint32_t color) {
  const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
  SetPixelV(hdc, (rect.left + xy_rel.x), (rect.top + xy_rel.y), color);
}

Rgb TextRenderSession::PixelSession::Get(const PixelPoint &xy_rel) {
  const auto ret = GetRaw(xy_rel);
  return Rgb{.r = GetRValue(ret), .g = GetGValue(ret), .b = GetBValue(ret)};
}

void TextRenderSession::PixelSession::Set(const PixelPoint &xy_rel,
                                          const Rgb color) {
  SetRaw(xy_rel, RGB(color.r, color.g, color.b));
}

TextRenderSession::TextRenderSession(const PixelLtwh &rect) : rect(rect) {
  const auto hdc = GraphicsSurfaceGdiTextSurface().dc;

  // Clear the rectangle, for two reasons:
  // • The caller is free to render multiple transparent strings on top
  //   of each other, so we can't rely on SetBkMode(hdc, OPAQUE).
  // • The next text might be shorter than the previous one in this
  //   rectangle.
  const PixelLtrb ltrb = rect;
  const RECT r = {ltrb.left, ltrb.top, ltrb.right, ltrb.bottom};
  auto black = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  const auto rect_filled_with_black = FillRect(hdc, &r, black);
  const auto background_mode_set_to_transparent = SetBkMode(hdc, TRANSPARENT);
  assert(rect_filled_with_black);
  assert(background_mode_set_to_transparent);
}

TextRenderSession::~TextRenderSession() {
  if (font_initial) {
    const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
    SelectObject(hdc, font_initial.value());
  }
  GraphicsSurfaceGdiTextUpdate(rect);
}

PixelSize TextRenderSession::RectSize() const { return {rect.w, rect.h}; }

void TextRenderSession::SetFont(FontId font) {
  if (font_cur != font) {
    const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
    auto font_prev = SelectObject(hdc, Fonts().ForID(font));
    if (!font_initial) {
      font_initial = font_prev;
    }
    font_cur = font;
  }
}

void TextRenderSession::SetColor(const Rgb color) {
  const COLORREF color_gdi = RGB(color.r, color.g, color.b);
  if (color_cur != color_gdi) {
    const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
    SetTextColor(hdc, color_gdi);
    color_cur = color_gdi;
  }
}

PixelSize TextRenderSession::Extent(std::string_view str) {
  return TextGDIExtent(std::nullopt, str);
}

void TextRenderSession::Put(const PixelPoint &topleft_rel, std::string_view str,
                            std::optional<Rgb> color) {
  WithWideUTF8(str, [&](const std::wstring_view str_w) {
    const auto hdc = GraphicsSurfaceGdiTextSurface().dc;
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

bool TextRender::Wipe() {
  if (!bounds || (bounds.w > (std::numeric_limits<int32_t>::max)()) ||
      (bounds.h > (std::numeric_limits<int32_t>::max)())) {
    assert(!"Invalid size for blank surface");
    return false;
  }
  const auto w = static_cast<int32_t>(bounds.w);
  const auto h = static_cast<int32_t>(bounds.h);
  return (GraphicsSurfaceGdiTextCreate(w, h, {0x00, 0x00, 0x00}) &&
          TextRenderPacked::Wipe());
}

std::optional<TextRenderSession> TextRender::Session(TextRenderRectId rect_id) {
  const auto &surf = GraphicsSurfaceGdiTextSurface();
  if ((surf.size != bounds) && !Wipe()) {
    return std::nullopt;
  }
  assert(rect_id < rects.size());
  return TextRenderSession{rects[rect_id].rect};
}

void TextRender::WipeBeforeNextRender() {
  TextRenderPacked::Wipe();

  // This also skips the needless creation of an uninitialized surface
  // during DirectDraw's init function.
  GraphicsSurfaceGdiTextSurface().size = {0, 0};
}

PixelSize TextRender::TextExtent(FontId font, std::string_view str) {
  return TextGDIExtent(Fonts().ForID(font), str);
}

void TextBackendCleanup() {
  Fonts().Cleanup();
  TextBackendGDICleanup();
}
