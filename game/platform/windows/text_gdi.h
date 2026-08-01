///
/// Text rendering via GDI
///

#pragma once

#include "gfx/graphics_backend.h"
#include "gfx/text_packed.h"

// Loads any required fonts from the game directory, and cleans them up at
// process termination.
void TextBackendGDIInit();
void TextBackendGDICleanup();

class TextRenderSession {
protected:
  // HFONT would require a cast of the value returned from SelectObject().
  // Thankfully, this type doesn't even require <windows.h>.
  using HdcObject = void *;

  std::optional<HdcObject> font_initial = std::nullopt;

  // A COLORREF created with the Rgb macro always has 0x00 in the topmost 8
  // bits.
  uint32_t color_cur = -1;

  FontId font_cur = FontId::Count;
  const PixelLtwh rect;

public:
  class PixelSession {
    friend class TextRenderSession;

    const PixelLtwh rect;

    PixelSession(const PixelLtwh rect) : rect(rect) {}

  public:
    uint32_t GetRaw(const PixelPoint &xy_rel);
    void SetRaw(const PixelPoint &xy_rel, uint32_t col);

    Rgb Get(const PixelPoint &xy_rel);
    void Set(const PixelPoint &xy_rel, const Rgb col);
  };

  PixelSize RectSize() const;
  void SetFont(FontId font);
  void SetColor(const Rgb color);
  PixelSize Extent(std::string_view str);
  void Put(const PixelPoint &topleft_rel, std::string_view str,
           std::optional<Rgb> color = std::nullopt);
  auto EditPixels(std::invocable<PixelSession &> auto f) {
    PixelSession p = {rect};
    return f(p);
  }

  TextRenderSession(const PixelLtwh &rect);
  ~TextRenderSession();
};

class TextRender : public TextRenderPacked {
  friend class TextRenderPacked;

  bool Wipe();
  std::optional<TextRenderSession> Session(TextRenderRectId rect_id);

public:
  void WipeBeforeNextRender();
  PixelSize TextExtent(FontId font, std::string_view str);
};
