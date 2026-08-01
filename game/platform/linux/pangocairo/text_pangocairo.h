///
/// Text rendering via Pango/Cairo
///

#pragma once

#include "gfx/graphics_backend.h"
#include "gfx/text_packed.h"

class TextRenderSession {
protected:
  PixelPoint tex_origin;
  PixelSize size;
  FontId font_cur = FontId::Count;
  Rgb color_cur = {0, 0, 0};

public:
  class PixelSession {
    friend class TextRenderSession;

    uint8_t *buf;
    int stride;

    PixelSession();
    uint32_t &PixelAt(const PixelPoint &xy_rel);

  public:
    uint32_t GetRaw(const PixelPoint &xy_rel);
    void SetRaw(const PixelPoint &xy_rel, uint32_t col);

    Rgb Get(const PixelPoint &xy_rel);
    void Set(const PixelPoint &xy_rel, const Rgb col);

    ~PixelSession();
  };

  PixelSize RectSize() const;
  void SetFont(FontId font);
  void SetColor(const Rgb &color);
  PixelSize Extent(std::string_view str);
  void Put(const PixelPoint &topleft_rel, std::string_view str,
           std::optional<Rgb> color = std::nullopt);
  auto EditPixels(std::invocable<PixelSession &> auto f) {
    PixelSession p;
    return f(p);
  }

  TextRenderSession(const PixelLtwh rect);
  ~TextRenderSession();
};

class TextRender : public TextRenderPacked {
  friend class TextRenderPacked;

  std::optional<TextRenderSession> Session(TextRenderRectId rect_id);

public:
  void WipeBeforeNextRender();
  PixelSize TextExtent(FontId font, std::string_view str);
};
