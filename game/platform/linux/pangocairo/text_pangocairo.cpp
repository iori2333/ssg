///
/// Text rendering via Pango/Cairo
///

// GCC 15 throws `error: redefinition of 'struct timespec'` if this appears
// after a module import.

#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>

#include "platform/text_backend.h"
#include "util/enum_array.h"
#include "util/guard.h"

constexpr auto kCairoFormat = CAIRO_FORMAT_ARGB32;

namespace {

constinit const util::EnumArray<const char *, FontId> kFontSpecs = {
    "MS Gothic,IPAMonaGothic Regular 14px",
    "MS Gothic,IPAMonaGothic Regular 16px",
    "MS Gothic,IPAMonaGothic Medium 24px",
    "MS Gothic,IPAMonaGothic Regular 10px",
};

} // namespace

struct PangoCairoFont {
  PangoFontDescription *desc = nullptr;
  cairo_hint_metrics_t hint_metrics = CAIRO_HINT_METRICS_DEFAULT;
};

struct PangoCairoState {
  cairo_surface_t *surf = nullptr;
  cairo_t *cr = nullptr;
  PangoLayout *layout = nullptr;

  PangoCairoState() {}
  PangoCairoState(cairo_surface_t *);
  PangoCairoState(PangoCairoState &&) noexcept;
  ~PangoCairoState();

  explicit operator bool() const;
  PangoCairoState &operator=(PangoCairoState &&) noexcept;

  void SetFont(const PangoCairoFont *);
  void SetText(std::string_view);
  PixelSize Extent(const PangoCairoFont *, std::string_view);
};

// Pango's hinting (which is, *of course*, controlled by a property in Cairo
// that overrides Fontconfig and can't be overridden itself) renders MS Gothic
// and IPAMonaGothic bitmaps 1 pixel lower than GDI… *except* for
//
// • MS Gothic at 10px and 16px, and
// • IPAMonaGothic at 10px,
//
// which happen to render pixel-perfectly. To check whether the game is
// actually using the optionally installable MS Gothic, we have to run Pango's
// font substitution, but the mere call to pango_font_map_load_font() already
// irrevocably applies Cairo's default metric hinting value to the respective
// entry in the Pango context's font map. Applying the Fontconfig substitutions
// is all we want here anyway.
bool MetricHintingNeededFor(PangoFontDescription *desc) {
  const auto *family_in = pango_font_description_get_family(desc);
  const auto size = pango_font_description_get_size(desc);
  if (!family_in) {
    return false;
  }
  FcPattern *pat_in = FcPatternCreate();
  if (!pat_in) {
    return false;
  }
  auto pat_in_guard = util::MakeGuard(pat_in, FcPatternDestroy);

  // PangoFc only exposes a conversion from `PangoFontDescription` to
  // `FcPattern`, but *of course* not the other way around...
  auto **families = g_strsplit(family_in, ",", -1);
  for (auto i = 0; families[i] != nullptr; i++) {
    auto *family = std::bit_cast<FcChar8 *>(families[i]);
    FcPatternAddString(pat_in, FC_FAMILY, family);
  }
  g_strfreev(families);

  FcConfigSubstitute(nullptr, pat_in, FcMatchPattern);
  FcDefaultSubstitute(pat_in);

  FcResult result;
  auto *pat_out = FcFontMatch(nullptr, pat_in, &result);
  auto pat_out_guard = util::MakeGuard(pat_out, FcPatternDestroy);

  char *family_out = nullptr;
  const auto matched = FcPatternGetString(
      pat_out, FC_FAMILY, 0, std::bit_cast<FcChar8 **>(&family_out));
  if (matched != FcResultMatch) {
    return false;
  }

  const auto is_ms_gothic = !strcmp(family_out, "MS Gothic");
  const auto is_ipamona_gothic = !strcmp(family_out, "IPAMonaGothic");
  switch (size) {
  case (10 * PANGO_SCALE):
    return (is_ms_gothic || is_ipamona_gothic);
  case (16 * PANGO_SCALE):
    return is_ms_gothic;
  }
  return false;
}

// State
// -----

TextRender &TextRenderer() {
  static TextRender renderer;
  return renderer;
}

class FontCache {
  util::EnumArray<PangoCairoFont, FontId> arr;

public:
  const PangoCairoFont &ForID(FontId font) {
    if (!arr[font].desc) {
      arr[font].desc = pango_font_description_from_string(kFontSpecs[font]);
      arr[font].hint_metrics =
          (MetricHintingNeededFor(arr[font].desc) ? CAIRO_HINT_METRICS_ON
                                                  : CAIRO_HINT_METRICS_OFF);
    }
    return arr[font];
  }

  void Cleanup() {
    for (auto &font : arr) {
      pango_font_description_free(font.desc);
      font.desc = nullptr;
    }
  }
};

PangoCairoState &PangoState() {
  static PangoCairoState state;
  return state;
}

FontCache &Fonts() {
  static FontCache fonts;
  return fonts;
}
// -----

PangoCairoState::PangoCairoState(cairo_surface_t *surf) : surf(surf) {
  if (!surf) {
    return;
  }
  cr = cairo_create(surf);
  assert(cr != nullptr); // Documentation says this will never fail
  layout = pango_cairo_create_layout(cr);
}

PangoCairoState::PangoCairoState(PangoCairoState &&other) noexcept {
  *this = std::move(other);
}

PangoCairoState::~PangoCairoState() {
  // Actually throws a warning if we pass a `nullptr`!
  if (layout) {
    g_object_unref(layout);
  }
  if (cr) {
    cairo_destroy(cr);
  }
  if (surf) {
    cairo_surface_destroy(surf);
  }
}

PangoCairoState::operator bool() const { return (cr && layout); }

PangoCairoState &PangoCairoState::operator=(PangoCairoState &&other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (layout) {
    g_object_unref(layout);
  }
  if (cr) {
    cairo_destroy(cr);
  }
  if (surf) {
    cairo_surface_destroy(surf);
  }
  surf = std::exchange(other.surf, nullptr);
  cr = std::exchange(other.cr, nullptr);
  layout = std::exchange(other.layout, nullptr);
  return *this;
};

void PangoCairoState::SetFont(const PangoCairoFont *font) {
  if (!font) {
    return;
  }
  auto *opts = cairo_font_options_create();
  assert(opts != nullptr); // Documentation says this will never fail
  cairo_get_font_options(cr, opts);

  // Each PangoLayout has its own copy of Cairo's font options, which we (of
  // *course*) can't get to from the outside, forcing us to recreate the
  // layout...
  if (cairo_font_options_get_hint_metrics(opts) != font->hint_metrics) {
    cairo_font_options_set_hint_metrics(opts, font->hint_metrics);
    cairo_set_font_options(cr, opts);
    g_object_unref(layout);
    layout = pango_cairo_create_layout(cr);
  }
  cairo_font_options_destroy(opts);
  pango_layout_set_font_description(layout, font->desc);
}

void PangoCairoState::SetText(std::string_view str) {
  const auto *in_buf = str.data();
  const auto in_size = str.size();
  if (g_utf8_validate_len(in_buf, in_size, nullptr)) {
    pango_layout_set_text(layout, in_buf, in_size);
  } else {
    gsize out_size;
    GError *error = nullptr;
    auto *out_buf = g_convert(in_buf, in_size, "UTF-8", "CP936", nullptr,
                              &out_size, &error // CP936 = GBK
    );
    pango_layout_set_text(layout, out_buf, out_size);
    g_free(out_buf);
  }
}

PixelSize PangoCairoState::Extent(const PangoCairoFont *font,
                                  std::string_view str) {
  PixelSize ret = {0, 0};
  SetFont(font);
  SetText(str);
  pango_layout_get_pixel_size(layout, &ret.w, &ret.h);
  return ret;
}

uint32_t &TextRenderSession::PixelSession::PixelAt(const PixelPoint &xy_rel) {
  return (std::bit_cast<uint32_t *>(buf + (stride * xy_rel.y)))[xy_rel.x];
}

uint32_t TextRenderSession::PixelSession::GetRaw(const PixelPoint &xy_rel) {
  return PixelAt(xy_rel);
}

void TextRenderSession::PixelSession::SetRaw(const PixelPoint &xy_rel,
                                             uint32_t color) {
  PixelAt(xy_rel) = color;
}

Rgb TextRenderSession::PixelSession::Get(const PixelPoint &xy_rel) {
  const auto ret = GetRaw(xy_rel);
  const uint8_t b = (ret >> 0);
  const uint8_t g = (ret >> 8);
  const uint8_t r = (ret >> 16);
  return Rgb{.r = r, .g = g, .b = b};
}

void TextRenderSession::PixelSession::Set(const PixelPoint &xy_rel,
                                          const Rgb c) {
  SetRaw(xy_rel, (0xFF000000u | (c.r << 16u) | (c.g << 8u) | c.b));
}

TextRenderSession::PixelSession::PixelSession() {
  cairo_surface_flush(PangoState().surf);
  buf = cairo_image_surface_get_data(PangoState().surf);
  stride = cairo_image_surface_get_stride(PangoState().surf);
}

TextRenderSession::PixelSession::~PixelSession() {
  cairo_surface_mark_dirty(PangoState().surf);
}

TextRenderSession::TextRenderSession(const PixelLtwh rect)
    : tex_origin(rect.left, rect.top), size(rect.w, rect.h) {}

TextRenderSession::~TextRenderSession() {
  cairo_surface_flush(PangoState().surf);
  const auto *buf = cairo_image_surface_get_data(PangoState().surf);
  const auto stride = cairo_image_surface_get_stride(PangoState().surf);
  const PixelLtwh subrect = {tex_origin.x, tex_origin.y, size.w, size.h};
  const auto sid = SurfaceId::Text;
  GraphicsSurfaceUpdate(sid, &subrect,
                        {std::bit_cast<const std::byte *>(buf), stride});
}

PixelSize TextRenderSession::RectSize() const { return size; }

void TextRenderSession::SetFont(FontId font) {
  if (font_cur != font) {
    PangoState().SetFont(&Fonts().ForID(font));
    font_cur = font;
  }
}

void TextRenderSession::SetColor(const Rgb &color) {
  if (color_cur != color) {
    cairo_set_source_rgb(PangoState().cr, (color.r / 255.0), (color.g / 255.0),
                         (color.b / 255.0));
    color_cur = color;
  }
}

PixelSize TextRenderSession::Extent(std::string_view str) {
  return PangoState().Extent(nullptr, str);
}

void TextRenderSession::Put(const PixelPoint &topleft_rel, std::string_view str,
                            std::optional<Rgb> color) {
  if (color) {
    SetColor(color.value());
  }
  PangoState().SetText(str);
  cairo_move_to(PangoState().cr, topleft_rel.x, topleft_rel.y);
  pango_cairo_show_layout(PangoState().cr, PangoState().layout);
}

std::optional<TextRenderSession> TextRender::Session(TextRenderRectId rect_id) {
  assert(rect_id < rects.size());
  const auto &rect = rects[rect_id].rect;

  PixelSize surf_size = {0, 0};
  if (PangoState().surf) {
    surf_size.w = cairo_image_surface_get_width(PangoState().surf);
    surf_size.h = cairo_image_surface_get_height(PangoState().surf);
  }
  if ((surf_size.w < rect.w) || (surf_size.h < rect.h)) {
    static_assert(std::same_as<decltype(bounds.w), int>);
    static_assert(std::same_as<decltype(bounds.h), int>);
    const auto w = std::max(surf_size.w, rect.w);
    const auto h = std::max(surf_size.h, rect.h);
    PangoState() = {cairo_image_surface_create(kCairoFormat, w, h)};
  } else if (PangoState()) {
    cairo_save(PangoState().cr);
    cairo_set_operator(PangoState().cr, CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(PangoState().cr, 0, 0, rect.w, rect.h);
    cairo_fill(PangoState().cr);
    cairo_restore(PangoState().cr);
  }
  if (!PangoState()) {
    return std::nullopt;
  }

  if (GraphicsSurfaceSize(SurfaceId::Text) != bounds) {
    TextRenderPacked::Wipe();
    if (!GraphicsSurfaceCreateUninitialized(SurfaceId::Text, bounds)) {
      return std::nullopt;
    }
  }
  return std::make_optional<TextRenderSession>(rect);
}

void TextRender::WipeBeforeNextRender() { TextRenderPacked::Wipe(); }

PixelSize TextRender::TextExtent(FontId font, std::string_view str) {
  // Luckily, Pango calculates extents just fine on 0×0 surfaces.
  if (!PangoState()) {
    PangoState() = {cairo_image_surface_create(kCairoFormat, 0, 0)};
  }
  return PangoState().Extent(&Fonts().ForID(font), str);
}

void TextBackendCleanup() {
  PangoState() = {};
  Fonts().Cleanup();
}
