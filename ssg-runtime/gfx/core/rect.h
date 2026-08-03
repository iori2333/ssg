///
/// Integer rectangles on the native pixel grid
///
#pragma once

#include "coords.h"

struct Rect {
  int left{};
  int top{};
  int right{};
  int bottom{};

  constexpr Rect() = default;
  constexpr Rect(int left, int top, int right, int bottom)
      : left(left), top(top), right(right), bottom(bottom) {}

  [[nodiscard]] static constexpr Rect FromLtwh(int left, int top, int w,
                                               int h) {
    return {left, top, left + w, top + h};
  }

  [[nodiscard]] static constexpr Rect FromPositionAndSize(PixelPoint position,
                                                          PixelPoint size) {
    return FromLtwh(position.x, position.y, size.x, size.y);
  }

  [[nodiscard]] constexpr int Width() const { return right - left; }
  [[nodiscard]] constexpr int Height() const { return bottom - top; }
  [[nodiscard]] constexpr PixelPoint Size() const {
    return {Width(), Height()};
  }

  [[nodiscard]] constexpr Rect operator+(PixelPoint offset) const {
    return {left + offset.x, top + offset.y, right + offset.x,
            bottom + offset.y};
  }
};
