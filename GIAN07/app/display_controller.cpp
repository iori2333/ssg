/// Runtime display configuration and graphics resource recovery.

#include <utility>

#include "display_controller.h"

#include "data/graphics_loader.h"
#include "gfx/constants.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "settings/config.h"

bool DisplayController::Initialize() {
  if (!GrpBackend_Enum()) {
    return false;
  }
  const auto result = Grp_InitOrFallback(config_.ToParams());
  if (!result) {
    return false;
  }
  config_.ApplyParams(result->live);
  GrpBackend_SetClip(GRP_RES_RECT);
  SetFrameRate(config_.fps_divisor);
  SetScreenshotEffort(config_.screenshot_effort);
  return true;
}

template <typename Modify> bool DisplayController::Apply(Modify &&modify) {
  const auto previous = config_.ToParams();
  auto requested = previous;
  std::forward<Modify>(modify)(requested);
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
  config_.ApplyParams(result->live);
  return !result->reload_surfaces || graphics_.Reload();
}

bool DisplayController::ToggleFullscreen() {
  return Apply(
      [](auto &params) { params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN; });
}

bool DisplayController::ToggleExclusiveFullscreen() {
  return Apply([](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN_EXCLUSIVE;
  });
}

bool DisplayController::ToggleScalingMode() {
  return Apply([](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  });
}

bool DisplayController::CycleScale(int_fast8_t delta, bool include_max) {
  return Apply([delta, include_max](auto &params) {
    const auto fullscreen = params.FullscreenFlags();
    if (fullscreen.fullscreen && !fullscreen.exclusive) {
      using Fit = GRAPHICS_FULLSCREEN_FIT;
      constexpr auto count = std::to_underlying(Fit::COUNT);
      const auto fit =
          (std::to_underlying(fullscreen.fit) + count + delta) % count;
      params.SetFlag(GRAPHICS_PARAM_FLAGS::FULLSCREEN_FIT, fit);
    } else if (!fullscreen.fullscreen) {
      const auto count = Grp_WindowScale4xMax() + include_max;
      if (count > 0) {
        params.window_scale_4x =
            (params.window_scale_4x + count + delta) % count;
      }
    }
  });
}

bool DisplayController::SelectApi(int8_t api) {
  return Apply([api](auto &params) { params.api = api; });
}

bool DisplayController::CycleApi() {
  const auto count = GrpBackend_APICount();
  if (count < 2) {
    return true;
  }
  return Apply(
      [count](auto &params) { params.api = (params.api + 1) % count; });
}

void DisplayController::SetFrameRate(uint8_t divisor) {
  config_.fps_divisor = divisor;
  Grp_FPSDivisor = divisor;
  if (divisor != 0) {
    turbo_restore_divisor_ = divisor;
  }
}

void DisplayController::SetScreenshotEffort(uint8_t effort) {
  config_.screenshot_effort = effort;
  Grp_ScreenshotSetEffort(effort);
}

void DisplayController::ToggleTurbo() {
  SetFrameRate(Grp_FPSDivisor == 0 ? turbo_restore_divisor_ : 0);
}
