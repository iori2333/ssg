/// Runtime display configuration and graphics resource recovery.

#include "display_controller.h"

#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "settings/config.h"
#include "util/enum_flags.h"
#include <cstdint>
#include <utility>

bool DisplayController::Initialize(GraphicsConfig &config) {
  if (!GraphicsBackendEnum()) {
    return false;
  }
  GraphicsParamFlags flags{};
  if (config.display_mode == DisplayMode::Fullscreen) {
    flags |= GraphicsParamFlags::Fullscreen;
  }
  if (config.fullscreen_mode == FullscreenMode::Exclusive) {
    flags |= GraphicsParamFlags::FullscreenExclusive;
  }
  if (config.scaling_mode == ScalingMode::Geometry) {
    flags |= GraphicsParamFlags::ScaleGeometry;
  }
  SetEnumFlag(flags, GraphicsParamFlags::FullscreenFit,
              std::to_underlying(config.fullscreen_fit));
  const GraphicsParams requested{
      .flags = flags,
      .api = static_cast<int8_t>(GraphicsBackendAPIID(config.graphics_api)),
      .window_scale_4x = config.window_scale_4x,
      .left = config.window_left,
      .top = config.window_top,
  };
  const auto result = GraphicsInitOrFallback(requested);
  if (!result) {
    return false;
  }
  params_ = result->live;
  const auto active_api = GraphicsBackendAPIString();
  params_.api = GraphicsBackendAPIID(active_api);
  const auto fullscreen = params_.FullscreenFlags();
  config.display_mode =
      fullscreen.fullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
  config.fullscreen_mode = fullscreen.exclusive ? FullscreenMode::Exclusive
                                                : FullscreenMode::Borderless;
  config.fullscreen_fit = fullscreen.fit;
  config.scaling_mode = params_.ScaleGeometry() ? ScalingMode::Geometry
                                                : ScalingMode::Framebuffer;
  config.graphics_api = active_api;
  config.window_scale_4x = params_.window_scale_4x;
  config.window_left = params_.left;
  config.window_top = params_.top;
  GraphicsBackendSetClip(kGameResolutionRect);
  SetFrameRate(config.fps_divisor);
  SetScreenshotEffort(config.screenshot_effort);
  return true;
}

bool DisplayController::Apply(GraphicsParams requested) {
  const auto previous = params_;
  if (previous == requested) {
    return true;
  }

  if (const auto topleft = WindowBackendTopleft()) {
    requested.left = topleft->first;
    requested.top = topleft->second;
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
  GraphicsParamFlags flags{};
  if (config.display_mode == DisplayMode::Fullscreen) {
    flags |= GraphicsParamFlags::Fullscreen;
  }
  if (config.fullscreen_mode == FullscreenMode::Exclusive) {
    flags |= GraphicsParamFlags::FullscreenExclusive;
  }
  if (config.scaling_mode == ScalingMode::Geometry) {
    flags |= GraphicsParamFlags::ScaleGeometry;
  }
  SetEnumFlag(flags, GraphicsParamFlags::FullscreenFit,
              std::to_underlying(config.fullscreen_fit));
  return Apply({
      .flags = flags,
      .api = static_cast<int8_t>(GraphicsBackendAPIID(config.graphics_api)),
      .window_scale_4x = config.window_scale_4x,
      .left = config.window_left,
      .top = config.window_top,
  });
}

void DisplayController::SetFrameRate(uint8_t divisor) {
  SetFrameRateDivisor(divisor);
}

void DisplayController::SetScreenshotEffort(uint8_t effort) {
  GraphicsScreenshotSetEffort(effort);
}
