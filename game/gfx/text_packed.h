///
/// Rectangle-packing text renderer
///
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "text.h"

class TextRenderPacked {
protected:
  struct RectAndContents {
    PixelLtwh rect;
    std::optional<std::string> contents;
  };

  PixelSize bounds = {};
  std::vector<PixelLtwh> spaces;
  std::vector<RectAndContents> rects;

  template <typename T> void SpaceAdd(T &&space) {
    if ((space.w > 0) && (space.h > 0)) {
      spaces.emplace_back(std::forward<T>(space));
    }
  }

  // Inserts a rectangle of the given size, expanding the empty space as
  // needed.
  PixelLtwh Insert(const PixelSize &subrect_size);

public:
  PixelLtwh Subrect(TextRenderRectId rect_id,
                    std::optional<PixelLtwh> maybe_subrect);

  TextRenderRectId Register(const PixelSize &size);

  bool Wipe();

  // Resets both bounds and empty spaces.
  void Clear();

  bool Blit(WindowPoint dst, TextRenderRectId rect_id,
            std::optional<PixelLtwh> subrect = std::nullopt);

  template <typename Self>
  bool Render(this Self &&self, WindowPoint dst, TextRenderRectId rect_id,
              std::string_view contents,
              std::invocable<TextRenderSession &> auto func,
              std::optional<PixelLtwh> subrect = std::nullopt) {
    assert(rect_id < self.rects.size());
    auto &rect = self.rects[rect_id];
    if (rect.contents != contents) {
      auto maybe_session = self.Session(rect_id);
      if (!maybe_session) {
        return false;
      }
      func(maybe_session.value());
      rect.contents = contents;
    }
    return self.Blit(dst, rect_id, subrect);
  }
};
