///
/// Common graphics interface, independent of a specific rendering API
///

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.

#include <algorithm>
#include <optional>
#include <ranges>
#include <string_view>

#include "graphics.h"
#include "graphics_system.h"
#include "image/screenshot.h"
#include "render/graphics_sdl.h"
#include "window/window_sdl.h"

namespace {

gfx::GraphicsState &State() { return gfx::ActiveGraphics().policy; }

} // namespace

void SetFrameRateDivisor(int divisor) { State().frame_rate_divisor = divisor; }

int FrameRateDivisor() { return State().frame_rate_divisor; }

// Screenshots
// -----------

void GraphicsScreenshotSetEffort(int effort) {
  image::ScreenshotSetEffort(effort);
}

void GraphicsScreenshotSetPrefix(std::string_view prefix) {
  image::ScreenshotSetPrefix(prefix);
}

void GraphicsRequestScreenshot(bool requested) {
  image::ScreenshotRequest(requested);
}
// -----------

// Returns the maximum 4× scaling factor for the game window on the current
// display.
int GraphicsWindowScale4xMax() {
  return GraphicsScale4xMaxFor(SdlGraphicsDisplaySize(false));
}

// Validates and clamps [params] to the supported ranges before passing them on
// to the SDL renderer.
std::optional<GraphicsInitResult>
GraphicsInit(std::optional<const GraphicsParams> maybe_prev,
             GraphicsParams params) {
  const auto api_count = GraphicsRenderDriverCount();
  if (params.render_driver < -1 ||
      ((api_count > 0) && (params.render_driver >= api_count))) {
    params.render_driver = -1;
  }
  params.exclusive_fullscreen &= params.fullscreen;
  if (params.fullscreen_fit >= GraphicsFullscreenFit::Count) {
    params.fullscreen_fit = GraphicsFullscreenFit::Integer;
  }
  params.window_scale_quarters =
      std::clamp(params.window_scale_quarters, 0, GraphicsWindowScale4xMax());
  return SdlGraphicsInit(maybe_prev, params);
}

// Calls GraphicsInit() with the given parameters and tries the remaining APIs
// on failure. Returns the actual configuration the backend was initialized
// with, or `std::nullopt` on failure.
std::optional<GraphicsInitResult>
GraphicsInitOrFallback(GraphicsParams params) {
  if (const auto ret = GraphicsInit(std::nullopt, params)) {
    return ret;
  }

  // Start with the defaults and try looking for a different working
  // configuration
  const auto api_count = GraphicsRenderDriverCount();

  const auto api_it =
      ((api_count > 0)
           ? std::views::iota(-1, api_count)
           : std::views::iota(params.render_driver, params.render_driver + 1));

  const auto failed_driver = params.render_driver;
  for (const auto api : api_it) {
    if (api == failed_driver) {
      continue;
    }
    params.render_driver = api;
    if (const auto ret = GraphicsInit(std::nullopt, params)) {
      return ret;
    }
  }
  return std::nullopt;
}

// Presents the current frame and handles a pending screenshot request.
void GraphicsFlip() { SdlGraphicsFlip(image::ScreenshotActive()); }
