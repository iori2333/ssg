///
/// Common graphics interface, independent of a specific rendering API
///
#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>

#include "coords.h"
#include "pixelformat.h"

#include "util/enum_flags.h"

// Setting the divisor to 0 disables frame rate limiting.
void SetFrameRateDivisor(uint8_t divisor);
uint8_t FrameRateDivisor();

// Paletted graphics //
// ----------------- //

struct Rgb {
  uint8_t r;
  uint8_t g;
  uint8_t b;

  [[nodiscard]] constexpr Rgba WithAlpha(uint8_t a) const {
    return Rgba{.r = r, .g = g, .b = b, .a = a};
  }

  constexpr bool operator==(const Rgb &other) const = default;
};
static_assert(sizeof(Rgb) == 3);

struct Palette : public std::array<Rgba, 256> {
  // Builds a new palette with the given fade [alpha] value applied onto the
  // given inclusive (!) range of colors. Returns the rest of the palette
  // unchanged.
  [[nodiscard]] Palette Fade(uint8_t alpha, uint8_t first = 0,
                             uint8_t last = 255) const;
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

  [[nodiscard]] constexpr uint8_t PaletteIndex() const {
    return (20 + r + (g * (Max + 1)) + (b * ((Max + 1) * (Max + 1))));
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

// ----------------- //

// Screenshots
// -----------

// 0 = BMP, 10 = max-effort WebP.
constexpr uint8_t kScreenshotEffortMax = 10;

void GraphicsScreenshotSetEffort(uint8_t effort);

// Required to enable the screenshot feature as a whole.
void GraphicsScreenshotSetPrefix(std::string_view prefix);
void GraphicsRequestScreenshot(bool requested);

struct SDL_Surface;

// Saves the given surface to a file with the screenshot prefix. [t_start]
// represents the very beginning of the backend's capturing process.
bool GraphicsScreenshotSave(SDL_Surface *src);
// -----------

enum class GraphicsFullscreenFit : uint8_t {
  // Scale to largest integer resolution
  Integer,

  // Scale to the largest resolution that fits the game's aspect ratio
  Aspect,

  // Stretch to entire screen, disregarding the aspect ratio
  Stretch,

  Count,
};

enum class GraphicsParamFlags : uint8_t {
  None = 0x00,
  Fullscreen = 0x01,
  FullscreenExclusive = 0x02,

  // A GraphicsFullscreenFit value
  FullscreenFit = (std::to_underlying(GraphicsFullscreenFit::Count) << 2),

  // Render at the window's resolution instead of at [kGameResolution]
  ScaleGeometry = 0x10,

  Mask = (Fullscreen | FullscreenExclusive | FullscreenFit | ScaleGeometry),
};

template <>
inline constexpr bool util::EnableEnumFlags<GraphicsParamFlags> = true;

struct GraphicsFullscreenFlags {
  bool fullscreen;
  bool exclusive;
  GraphicsFullscreenFit fit;

  std::strong_ordering
  operator<=>(const GraphicsFullscreenFlags &) const = default;
};

constexpr auto kGraphicsTopleftUndefined = std::numeric_limits<int16_t>::min();

struct GraphicsParams {
  GraphicsParamFlags flags;
  int8_t api;              // Negative = "use default API"
  uint8_t window_scale_4x; // Scale factor in window mode ×4. 0 = fit display.

  // Across all displays. Can be [kGraphicsTopleftUndefined], in which case
  // the window backend should pick a reasonable default position.
  int16_t left;
  int16_t top;

  std::strong_ordering operator<=>(const GraphicsParams &) const = default;

  [[nodiscard]] GraphicsFullscreenFlags FullscreenFlags() const;
  [[nodiscard]] bool ScaleGeometry() const;
  [[nodiscard]] uint8_t Scale4x() const;
  [[nodiscard]] WindowSize ScaledRes() const;

  void SetFlag(GraphicsParamFlags flag,
               std::underlying_type_t<GraphicsParamFlags> value);
};

// Returns the maximum 4× scaling factor for the game window on the current
// display.
uint8_t GraphicsWindowScale4xMax();

struct GraphicsInitResult {
  GraphicsParams live;
  bool reload_surfaces;

  static std::optional<GraphicsInitResult>
  From(std::optional<GraphicsParams> o) {
    return std::move(o).transform([](auto &&o) {
      return GraphicsInitResult{.live = o, .reload_surfaces = false};
    });
  }
};

// Validates and clamps [params] to the supported ranges before passing them on
// to GraphicsBackendInit().
std::optional<GraphicsInitResult>
GraphicsInit(std::optional<const GraphicsParams> maybe_prev,
             GraphicsParams params);

// Calls GraphicsInit() with the given parameters and tries the remaining APIs
// on failure. Returns the actual configuration the backend was initialized
// with, or `std::nullopt` on failure.
std::optional<GraphicsInitResult> GraphicsInitOrFallback(GraphicsParams params);

// Wraps screenshot handling around GraphicsBackendFlip().
void GraphicsFlip();
