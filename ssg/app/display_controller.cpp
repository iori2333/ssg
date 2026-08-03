/// Runtime display configuration and graphics resource recovery.

#include <cstdint>
#include <utility>

#include "display_controller.h"

#include "data/graphics_loader.h"
#include "gfx/core/constants.h"
#include "gfx/graphics.h"
#include "gfx/text/text_renderer.h"
#include "gfx/window/window.h"
#include "settings/config.h"

namespace {

GraphicsParams ParamsFromConfig(const GraphicsConfig &config) {
  return {
      .fullscreen = config.display_mode == DisplayMode::Fullscreen,
      .exclusive_fullscreen =
          config.fullscreen_mode == FullscreenMode::Exclusive,
      .fullscreen_fit = config.fullscreen_fit,
      .scale_geometry = config.scaling_mode == ScalingMode::Geometry,
      .render_driver = GraphicsRenderDriverId(config.graphics_api),
      .window_scale_quarters = config.window_scale_4x,
      .window_left = config.window_left,
      .window_top = config.window_top,
  };
}

} // namespace

bool DisplayController::Initialize(GraphicsConfig &config) {
  const auto requested = ParamsFromConfig(config);
  const auto result = GraphicsInitOrFallback(requested);
  if (!result) {
    return false;
  }
  params_ = result->live;
  const auto active_api = GraphicsActiveRenderDriver();
  params_.render_driver = GraphicsRenderDriverId(active_api);
  config.display_mode =
      params_.fullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
  config.fullscreen_mode = params_.exclusive_fullscreen
                               ? FullscreenMode::Exclusive
                               : FullscreenMode::Borderless;
  config.fullscreen_fit = params_.fullscreen_fit;
  config.scaling_mode =
      params_.scale_geometry ? ScalingMode::Geometry : ScalingMode::Framebuffer;
  config.graphics_api = active_api;
  config.window_scale_4x = params_.window_scale_quarters;
  config.window_left = params_.window_left;
  config.window_top = params_.window_top;
  GraphicsSetClip(kGameResolutionRect);
  SetFrameRate(config.fps_divisor);
  SetScreenshotEffort(config.screenshot_effort);
  return true;
}

bool DisplayController::Apply(GraphicsParams requested) {
  const auto previous = params_;
  if (previous == requested) {
    return true;
  }

  if (const auto topleft = WindowPosition()) {
    requested.window_left = topleft->first;
    requested.window_top = topleft->second;
  }

  auto result = GraphicsInit(previous, requested);
  if (!result) {
    result = GraphicsInitOrFallback(previous);
  }
  if (!result) {
    return false;
  }

  TextRenderer().WipeBeforeNextRender();
  params_ = result->live;
  return !result->reload_surfaces || graphics_.Reload();
}

bool DisplayController::ApplyConfig(const GraphicsConfig &config) {
  return Apply(ParamsFromConfig(config));
}

void DisplayController::SetFrameRate(int divisor) {
  SetFrameRateDivisor(divisor);
}

void DisplayController::SetScreenshotEffort(int effort) {
  GraphicsScreenshotSetEffort(effort);
}
