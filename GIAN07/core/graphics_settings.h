///
/// Graphics settings — hot-reloadable display configuration helpers.
///
#pragma once

#include "core/config.h"

#include "data/gfx_manager.h"
#include "gfx/graphics.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"

inline void XGrpTry(GraphicsConfig &gfx_cfg, const GRAPHICS_PARAMS &prev, GRAPHICS_PARAMS &params) {
  if (prev == params) {
    return;
  }
  if (const auto maybe_topleft = WndBackend_Topleft()) {
    const auto &topleft = maybe_topleft.value();
    params.left = topleft.first;
    params.top = topleft.second;
  }
  auto maybe_result = Grp_Init(prev, params);
  if (!maybe_result) {
    // Try resetting to the previous configuration, or, if necessary,
    // attempt anything to get graphics back working again.
    maybe_result = Grp_InitOrFallback(prev);
  }
  if (maybe_result) {
    const auto &result = maybe_result.value();
    TextObj.WipeBeforeNextRender();
    gfx_cfg.GraphicsParamsApply(result.live);
    if (result.reload_surfaces) {
      gfx.ReloadStage();
    }
  }
}

// Tries the graphics configuration that results from applying the given
// [patch_func] onto the current configuration, and updates all subsystems
// accordingly.
void XGrpTry(GraphicsConfig &gfx_cfg, std::invocable<GRAPHICS_PARAMS &> auto &&patch_func) {
  const auto prev = gfx_cfg.GraphicsParams();
  auto params = prev;
  patch_func(params);
  XGrpTry(gfx_cfg, prev, params);
}

inline void XGrpTryCycleScale(GraphicsConfig &gfx_cfg, int_fast8_t delta, bool include_max) {
  XGrpTry(gfx_cfg, [&](auto &params) {
    const auto fs = params.FullscreenFlags();
    if (fs.fullscreen && !fs.exclusive) {
      using FIT = GRAPHICS_FULLSCREEN_FIT;
      constexpr auto max = std::to_underlying(FIT::COUNT);
      const auto fit = ((std::to_underlying(fs.fit) + max + delta) % max);
      params.SetFlag(GRAPHICS_PARAM_FLAGS::FULLSCREEN_FIT, fit);
    } else if (!fs.fullscreen) {
      const auto max = (Grp_WindowScale4xMax() + include_max);
      params.window_scale_4x = ((params.window_scale_4x + max + delta) % max);
    }
  });
}

inline void XGrpTryCycleDisp(GraphicsConfig &gfx_cfg) {
  XGrpTry(gfx_cfg,
      [](auto &params) { params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN; });
}

inline void XGrpTryCycleScMode(GraphicsConfig &gfx_cfg) {
  XGrpTry(gfx_cfg, [](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  });
}
