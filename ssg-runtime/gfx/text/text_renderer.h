///
/// Text rendering and cached text regions.
///

#pragma once

#include <concepts>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "text.h"

#include "gfx/graphics.h"

class TextRenderSession {
  PixelPoint texture_origin_;
  PixelPoint size_;
  FontId font_ = FontId::Count;
  Rgb color_ = {.r = 255, .g = 255, .b = 255};

public:
  class PixelSession {
    uint8_t *pixels_ = nullptr;
    int pitch_ = 0;
    bool locked_ = false;
    PixelPoint size_{};

    [[nodiscard]] uint32_t &PixelAt(const PixelPoint &xy_rel) const;

  public:
    explicit PixelSession(PixelPoint size);
    ~PixelSession();

    PixelSession(const PixelSession &) = delete;
    PixelSession &operator=(const PixelSession &) = delete;

    [[nodiscard]] uint32_t GetRaw(const PixelPoint &xy_rel) const;
    void SetRaw(const PixelPoint &xy_rel, uint32_t color) const;

    [[nodiscard]] Rgb Get(const PixelPoint &xy_rel) const;
    void Set(const PixelPoint &xy_rel, Rgb color) const;
  };

  explicit TextRenderSession(Rect rect);
  ~TextRenderSession();

  TextRenderSession(const TextRenderSession &) = delete;
  TextRenderSession &operator=(const TextRenderSession &) = delete;

  [[nodiscard]] PixelPoint RectSize() const;
  void SetFont(FontId font);
  void SetColor(Rgb color);
  [[nodiscard]] PixelPoint Extent(std::string_view text) const;
  void Put(const PixelPoint &topleft_rel, std::string_view text,
           std::optional<Rgb> color = std::nullopt);

  auto EditPixels(std::invocable<PixelSession &> auto func) {
    PixelSession pixels(size_);
    return func(pixels);
  }
};

class TextRender {
  struct Entry {
    Rect rect;
    std::optional<std::string> cache_key;
  };

  PixelPoint bounds_{};
  std::vector<Rect> spaces_;
  std::vector<Entry> entries_;
  uint64_t generation_ = 1;

  template <typename T> void AddSpace(T &&space) {
    if ((space.Width() > 0) && (space.Height() > 0)) {
      spaces_.emplace_back(std::forward<T>(space));
    }
  }

  [[nodiscard]] std::optional<Rect> Insert(PixelPoint size);
  [[nodiscard]] Entry *Find(TextRenderRectId id);
  [[nodiscard]] const Entry *Find(TextRenderRectId id) const;
  [[nodiscard]] std::optional<Rect> Subrect(TextRenderRectId id,
                                            std::optional<Rect> subrect) const;
  std::optional<TextRenderSession> Session(TextRenderRectId rect_id);

public:
  [[nodiscard]] TextRenderRectId Register(PixelPoint size);
  void Wipe();
  void Clear();
  bool Blit(PixelPoint dst, TextRenderRectId rect_id,
            std::optional<Rect> subrect = std::nullopt);

  template <typename Func>
  bool Render(PixelPoint dst, TextRenderRectId rect_id,
              std::string_view cache_key, Func &&render,
              std::optional<Rect> subrect = std::nullopt) {
    auto *entry = Find(rect_id);
    if (entry == nullptr) {
      return false;
    }
    if (entry->cache_key != cache_key) {
      auto session = Session(rect_id);
      if (!session) {
        return false;
      }
      std::forward<Func>(render)(*session);
      entry->cache_key = cache_key;
    }
    return Blit(dst, rect_id, subrect);
  }

  void WipeBeforeNextRender();
  [[nodiscard]] static PixelPoint TextExtent(FontId font,
                                             std::string_view text);
};

[[nodiscard]] bool TextInitialize(std::string_view language);
[[nodiscard]] bool TextSetLanguage(std::string_view language);
void TextCleanup();

TextRender &TextRenderer();
