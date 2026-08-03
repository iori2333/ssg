///
/// Common graphics interface, independent of a specific rendering API
///
#pragma once

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include "core/constants.h"
#include "core/coords.h"
#include "core/pixelformat.h"
#include "core/rect.h"

// Setting the divisor to 0 disables frame rate limiting.

void SetFrameRateDivisor(int divisor);
int FrameRateDivisor();

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

// ----------------- //

// Screenshots
// -----------

// 0 = BMP, 10 = max-effort WebP.
constexpr int kScreenshotEffortMax = 10;

void GraphicsScreenshotSetEffort(int effort);

// Required to enable the screenshot feature as a whole.
void GraphicsScreenshotSetPrefix(std::string_view prefix);
void GraphicsRequestScreenshot(bool requested);

// -----------

// Rendering drivers
// -----------------

int GraphicsRenderDriverCount();
std::string_view GraphicsRenderDriverLabel(std::string_view driver);
int GraphicsRenderDriverId(std::string_view driver);
std::string_view GraphicsRenderDriverName(int id);
std::string_view GraphicsActiveRenderDriver();
// -----------------

enum class GraphicsFullscreenFit : uint8_t {
  // Scale to largest integer resolution
  Integer,

  // Scale to the largest resolution that fits the game's aspect ratio
  Aspect,

  // Stretch to entire screen, disregarding the aspect ratio
  Stretch,

  Count,
};

constexpr auto kGraphicsTopleftUndefined = std::numeric_limits<int>::min();

struct GraphicsParams {
  bool fullscreen = false;
  bool exclusive_fullscreen = false;
  GraphicsFullscreenFit fullscreen_fit = GraphicsFullscreenFit::Integer;
  bool scale_geometry = false;
  int render_driver = -1;        // Negative = use the default renderer.
  int window_scale_quarters = 0; // 0 = fit the current display.

  // Across all displays. Can be [kGraphicsTopleftUndefined], in which case
  // the window backend should pick a reasonable default position.
  int window_left = kGraphicsTopleftUndefined;
  int window_top = kGraphicsTopleftUndefined;

  std::strong_ordering operator<=>(const GraphicsParams &) const = default;

  [[nodiscard]] int Scale4x() const;
  [[nodiscard]] PixelPoint ScaledRes() const;
};

// Returns the maximum 4× scaling factor for the game window on the current
// display.
int GraphicsWindowScale4xMax();

struct GraphicsInitResult {
  GraphicsParams live;
  bool reload_surfaces;
};

// Validates and clamps [params] to the supported ranges before passing them on
// to the SDL renderer.
std::optional<GraphicsInitResult>
GraphicsInit(std::optional<const GraphicsParams> maybe_prev,
             GraphicsParams params);

// Calls GraphicsInit() with the given parameters and tries the remaining APIs
// on failure. Returns the actual configuration the backend was initialized
// with, or `std::nullopt` on failure.
std::optional<GraphicsInitResult> GraphicsInitOrFallback(GraphicsParams params);

// Presents the current frame and handles a pending screenshot request.
void GraphicsFlip();

void GraphicsCleanup();

void GraphicsClear(Rgb color = {.r = 0, .g = 0, .b = 0});
void GraphicsSetClip(const Rect &rect);

// Textures
// --------

struct BmpOwned;

bool GraphicsSurfaceCreateUninitialized(SurfaceId sid, const PixelPoint &size);
bool GraphicsSurfaceLoad(SurfaceId sid, BmpOwned bmp);
bool GraphicsSurfaceUpdate(SurfaceId sid, const Rect *subrect,
                           std::tuple<const uint8_t *, size_t> pixels) noexcept;
PixelPoint GraphicsSurfaceSize(SurfaceId sid);
bool GraphicsSurfaceBlit(PixelPoint topleft, SurfaceId sid, const Rect &src);
void GraphicsSurfaceBlitOpaque(PixelPoint topleft, SurfaceId sid,
                               const Rect &src);
void GraphicsSurfaceSetColorMod(SurfaceId sid, uint8_t r, uint8_t g, uint8_t b);
// --------

// Geometry vertex types
// ---------------------

struct VertexXy {
  float x{};
  float y{};

  [[nodiscard]] constexpr VertexXy DivInt(int scalar) const {
    return {
        .x = static_cast<float>(static_cast<int>(x) / scalar),
        .y = static_cast<float>(static_cast<int>(y) / scalar),
    };
  }

  constexpr VertexXy operator+(const VertexXy &other) const {
    return {(x + other.x), (y + other.y)};
  }
};

struct VertexRgba {
  float r;
  float g;
  float b;
  float a;

  VertexRgba() = default;
  VertexRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r / 255.0F), g(g / 255.0F), b(b / 255.0F), a(a / 255.0F) {}
  VertexRgba(const Rgba &o)
      : r(o.r / 255.0F), g(o.g / 255.0F), b(o.b / 255.0F), a(o.a / 255.0F) {}
};

template <size_t N = std::dynamic_extent>
using VertexXySpan = std::span<const VertexXy, N>;
template <size_t N = std::dynamic_extent>
using VertexRgbaSpan = std::span<const VertexRgba, N>;

enum class TrianglePrimitive : uint8_t { Fan, Strip, Count };
// ---------------------

// Software pixel access
// ---------------------

bool GraphicsPixelAccessStart();
bool GraphicsPixelAccessEnd();
std::tuple<uint8_t *, size_t> GraphicsPixelAccessLock();
void GraphicsPixelAccessUnlock();

template <typename Func>
[[nodiscard]] bool GraphicsPixelAccessEdit(Func &&func) {
  const auto [pixels, pitch] = GraphicsPixelAccessLock();
  if (pitch == 0) {
    return false;
  }
  std::forward<Func>(func)(pixels, pitch);
  GraphicsPixelAccessUnlock();
  return true;
}
// ---------------------
