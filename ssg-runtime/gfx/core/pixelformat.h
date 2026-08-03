///
/// Pixel formats and colors
///
#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <ranges>

// Same as the standard Win32 PALETTEENTRY structure.

struct Rgba {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;

  constexpr bool operator==(const Rgba &other) const = default;
};

// Same as the standard Win32 RGBQUAD structure.
struct Bgra {
  uint8_t b;
  uint8_t g;
  uint8_t r;
  uint8_t a;

  constexpr bool operator==(const Bgra &other) const = default;
};

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  [[nodiscard]] constexpr Rgba WithAlpha(uint8_t a) const {
    return Rgba{.r = r, .g = g, .b = b, .a = a};
  }

  constexpr bool operator==(const Rgb &other) const = default;
};

// (6 * 6 * 6) = 216 standard colors, available in both channeled and
// palettized modes.
struct Rgb216 {
  static constexpr uint8_t Max = 5;

  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  constexpr Rgb216() = default;
  constexpr Rgb216(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {
    if ((r > Max) || (g > Max) || (b > Max)) {
      throw "216-color component out of range";
    }
  }

  static Rgb216 Clamped(uint8_t r, uint8_t g, uint8_t b) {
    return Rgb216{std::min(r, Max), std::min(g, Max), std::min(b, Max)};
  }

  [[nodiscard]] constexpr Rgb ToRgb() const {
    return Rgb{
        .r = static_cast<uint8_t>(r * 50U),
        .g = static_cast<uint8_t>(g * 50U),
        .b = static_cast<uint8_t>(b * 50U),
    };
  }

  static void ForEach(std::invocable<const Rgb216 &> auto &&func) {
    constexpr uint8_t start = 0;
    constexpr uint8_t end = (Max + 1);
    for (const auto r : std::views::iota(start, end)) {
      for (const auto g : std::views::iota(start, end)) {
        for (const auto b : std::views::iota(start, end)) {
          func(Rgb216{r, g, b});
        }
      }
    }
  }
};
