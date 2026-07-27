///
/// Graphics settings - hot-reloadable display configuration helpers.
///
#pragma once

#include <concepts>
#include <cstdint>
#include <utility>

#include "data/graphics_loader.h"
#include "gfx/graphics.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "settings/config.h"

namespace graphics_settings {

inline void TryApply(GraphicsConfig &config, data::GraphicsLoader &loader,
                     const GRAPHICS_PARAMS &previous,
                     GRAPHICS_PARAMS &requested) {
  if (previous == requested) {
    return;
  }
  if (const auto maybe_topleft = WndBackend_Topleft()) {
    const auto &topleft = maybe_topleft.value();
    requested.left = topleft.first;
    requested.top = topleft.second;
  }
  auto maybe_result = Grp_Init(previous, requested);
  if (!maybe_result) {
    // Try resetting to the previous configuration, or, if necessary,
    // attempt anything to get graphics back working again.
    maybe_result = Grp_InitOrFallback(previous);
  }
  if (maybe_result) {
    const auto &result = maybe_result.value();
    TextObj.WipeBeforeNextRender();
    config.ApplyParams(result.live);
    if (result.reload_surfaces) {
      (void)loader.Reload();
    }
  }
}

// Tries the graphics configuration that results from applying the given
// [modify] onto the current configuration, and updates all subsystems
// accordingly.
void TryApply(GraphicsConfig &config, data::GraphicsLoader &loader,
              std::invocable<GRAPHICS_PARAMS &> auto &&modify) {
  const auto previous = config.ToParams();
  auto requested = previous;
  modify(requested);
  TryApply(config, loader, previous, requested);
}

inline void CycleScale(GraphicsConfig &config, data::GraphicsLoader &loader,
                       int_fast8_t delta, bool include_max) {
  TryApply(config, loader, [&](auto &params) {
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

inline void ToggleFullscreen(GraphicsConfig &config,
                             data::GraphicsLoader &loader) {
  TryApply(config, loader, [](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN;
  });
}

inline void ToggleScalingMode(GraphicsConfig &config,
                              data::GraphicsLoader &loader) {
  TryApply(config, loader, [](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  });
}

} // namespace graphics_settings
