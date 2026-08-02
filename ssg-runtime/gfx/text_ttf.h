///
/// Cross-platform text rendering via SDL_ttf
///

#pragma once

#include <concepts>
#include <cstdint>
#include <optional>
#include <string_view>

#include "graphics_backend.h"
#include "text_packed.h"

class TextRenderSession {
  PixelPoint texture_origin_;
  PixelSize size_;
  FontId font_ = FontId::Count;
  Rgb color_ = {.r = 255, .g = 255, .b = 255};

public:
  class PixelSession {
    uint8_t *pixels_ = nullptr;
    int pitch_ = 0;
    bool locked_ = false;

    [[nodiscard]] uint32_t &PixelAt(const PixelPoint &xy_rel) const;

  public:
    PixelSession();
    ~PixelSession();

    PixelSession(const PixelSession &) = delete;
    PixelSession &operator=(const PixelSession &) = delete;

    [[nodiscard]] uint32_t GetRaw(const PixelPoint &xy_rel) const;
    void SetRaw(const PixelPoint &xy_rel, uint32_t color) const;

    [[nodiscard]] Rgb Get(const PixelPoint &xy_rel) const;
    void Set(const PixelPoint &xy_rel, Rgb color) const;
  };

  explicit TextRenderSession(PixelLtwh rect);
  ~TextRenderSession();

  TextRenderSession(const TextRenderSession &) = delete;
  TextRenderSession &operator=(const TextRenderSession &) = delete;

  [[nodiscard]] PixelSize RectSize() const;
  void SetFont(FontId font);
  void SetColor(Rgb color);
  [[nodiscard]] static PixelSize Extent(std::string_view text);
  void Put(const PixelPoint &topleft_rel, std::string_view text,
           std::optional<Rgb> color = std::nullopt);

  auto EditPixels(std::invocable<PixelSession &> auto func) {
    PixelSession pixels;
    return func(pixels);
  }
};

class TextRender : public TextRenderPacked {
  friend class TextRenderPacked;

  std::optional<TextRenderSession> Session(TextRenderRectId rect_id);

public:
  void WipeBeforeNextRender();
  [[nodiscard]] static PixelSize TextExtent(FontId font, std::string_view text);
};

[[nodiscard]] bool TextBackendInitialize(std::string_view language);
[[nodiscard]] bool TextBackendSetLanguage(std::string_view language);
void TextBackendCleanup();

TextRender &TextRenderer();
