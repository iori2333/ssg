/// Runtime display configuration and graphics resource recovery.

#include "display_controller.h"

#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "settings/config.h"

bool DisplayController::Initialize(GraphicsConfig &config) {
  if (!GrpBackend_Enum()) {
    return false;
  }
  GRAPHICS_PARAM_FLAGS flags{};
  if (config.display_mode == DisplayMode::Fullscreen) {
    flags |= GRAPHICS_PARAM_FLAGS::FULLSCREEN;
  }
  if (config.fullscreen_mode == FullscreenMode::Exclusive) {
    flags |= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
  }
  if (config.scaling_mode == ScalingMode::Geometry) {
    flags |= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  }
  EnumFlagSet(flags, GRAPHICS_PARAM_FLAGS::FULLSCREEN_FIT,
              std::to_underlying(config.fullscreen_fit));
  const GRAPHICS_PARAMS requested{
      .flags = flags,
      .device_id = config.device_id,
      .api = GrpBackend_APIID(config.graphics_api),
      .window_scale_4x = config.window_scale_4x,
      .left = config.window_left,
      .top = config.window_top,
  };
  const auto result = Grp_InitOrFallback(requested);
  if (!result) {
    return false;
  }
  params_ = result->live;
  const auto fullscreen = params_.FullscreenFlags();
  config.display_mode =
      fullscreen.fullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed;
  config.fullscreen_mode = fullscreen.exclusive ? FullscreenMode::Exclusive
                                                : FullscreenMode::Borderless;
  config.fullscreen_fit = fullscreen.fit;
  config.scaling_mode = params_.ScaleGeometry() ? ScalingMode::Geometry
                                                : ScalingMode::Framebuffer;
  config.device_id = params_.device_id;
  config.graphics_api = GrpBackend_APIString(params_.api);
  config.window_scale_4x = params_.window_scale_4x;
  config.window_left = params_.left;
  config.window_top = params_.top;
  GrpBackend_SetClip(GRP_RES_RECT);
  SetFrameRate(config.fps_divisor);
  SetScreenshotEffort(config.screenshot_effort);
  return true;
}

bool DisplayController::Apply(GRAPHICS_PARAMS requested) {
  const auto previous = params_;
  if (previous == requested) {
    return true;
  }

  if (const auto topleft = WndBackend_Topleft()) {
    requested.left = topleft->first;
    requested.top = topleft->second;
  }

  auto result = Grp_Init(previous, requested);
  if (!result) {
    result = Grp_InitOrFallback(previous);
  }
  if (!result) {
    return false;
  }

  TextObj.WipeBeforeNextRender();
  params_ = result->live;
  return !result->reload_surfaces || graphics_.Reload();
}

bool DisplayController::ApplyConfig(const GraphicsConfig &config) {
  GRAPHICS_PARAM_FLAGS flags{};
  if (config.display_mode == DisplayMode::Fullscreen) {
    flags |= GRAPHICS_PARAM_FLAGS::FULLSCREEN;
  }
  if (config.fullscreen_mode == FullscreenMode::Exclusive) {
    flags |= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
  }
  if (config.scaling_mode == ScalingMode::Geometry) {
    flags |= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  }
  EnumFlagSet(flags, GRAPHICS_PARAM_FLAGS::FULLSCREEN_FIT,
              std::to_underlying(config.fullscreen_fit));
  return Apply({
      .flags = flags,
      .device_id = config.device_id,
      .api = GrpBackend_APIID(config.graphics_api),
      .window_scale_4x = config.window_scale_4x,
      .left = config.window_left,
      .top = config.window_top,
  });
}

void DisplayController::SetFrameRate(uint8_t divisor) {
  Grp_FPSDivisor = divisor;
}

void DisplayController::SetScreenshotEffort(uint8_t effort) {
  Grp_ScreenshotSetEffort(effort);
}
