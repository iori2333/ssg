///
/// Common graphics interface, independent of a specific rendering API
///
#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
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

// Screenshots
// -----------

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

// Maximum integer 4× scaling factor that fits [display_size].
constexpr int GraphicsScale4xMaxFor(const PixelPoint &display_size) {
  const auto factors = ((display_size * 4) / kGameResolution);
  return std::max(1, std::min(factors.x, factors.y));
}

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

  [[nodiscard]] constexpr int Scale4x() const {
    if (fullscreen) {
      return exclusive_fullscreen ? 4 : 0;
    }
    return window_scale_quarters;
  }

  [[nodiscard]] constexpr PixelPoint ScaledRes(PixelPoint display_size) const {
    if (fullscreen) {
      if (exclusive_fullscreen) {
        return kGameResolution;
      }
      switch (fullscreen_fit) {
      case GraphicsFullscreenFit::Integer: {
        const auto factors = (display_size / kGameResolution);
        return (kGameResolution * std::min(factors.x, factors.y));
      }
      case GraphicsFullscreenFit::Aspect: {
        const auto factor_w =
            (static_cast<float>(display_size.x) / kGameResolution.x);
        const auto factor_h =
            (static_cast<float>(display_size.y) / kGameResolution.y);
        const auto scale = std::min(factor_w, factor_h);
        return {
            .x = static_cast<int>(kGameResolution.x * scale),
            .y = static_cast<int>(kGameResolution.y * scale),
        };
      }
      case GraphicsFullscreenFit::Stretch:
        return display_size;
      case GraphicsFullscreenFit::Count:
        std::unreachable();
      }
    }
    const auto scale = ((window_scale_quarters == 0)
                            ? GraphicsScale4xMaxFor(display_size)
                            : window_scale_quarters);
    return ((kGameResolution * scale) / 4);
  }
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
