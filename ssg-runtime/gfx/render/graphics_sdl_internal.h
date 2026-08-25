///
/// Internal helpers shared across the SDL renderer backends
/// (graphics_sdl.cpp, graphics_sdl_geometry.cpp, graphics_sdl_pixels.cpp).
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

#include "geometry.h"

#include "gfx/core/constants.h"
#include "gfx/core/rect.h"
#include "gfx/graphics_system.h"
#include "sys/log.h"
#include "util/enum_array.h"

using SdlColor = SDL_FColor;

constexpr auto kLogCat = logging::Channel::Graphics;

inline gfx::RendererState &RenderState() { return gfx::ActiveGraphics().renderer; }

// Compile-time index buffers
// --------------------------

using IndexType = uint8_t;

constexpr int TriangleIndexCount(size_t vertex_count) {
  return ((vertex_count - 3 + 1) * 3);
}

constexpr auto kTriangleFan = ([] {
  constexpr IndexType max = kMaxTriangles;
  std::array<IndexType, TriangleIndexCount(max)> ret{};
  auto ret_p = ret.begin();
  for (const auto &i : std::views::iota(0U, (max - 2U))) {
    *(ret_p++) = 0;
    *(ret_p++) = (i + 1);
    *(ret_p++) = (i + 2);
  }
  return ret;
})();

constexpr auto kTriangleStrip = ([] {
  constexpr IndexType max = kMaxTriangles;
  std::array<IndexType, TriangleIndexCount(max)> ret{};
  auto ret_p = ret.begin();
  for (const auto &i : std::views::iota(0U, (max - 2U))) {
    *(ret_p++) = (i + 0);
    *(ret_p++) = (i + 1);
    *(ret_p++) = (i + 2);
  }
  return ret;
})();

constexpr util::EnumArray<std::span<const IndexType>, TrianglePrimitive>
    kTriangleIndices = {kTriangleFan, kTriangleStrip};
// --------------------------

template <typename SdlRect> SdlRect HelpRectTo(const Rect &o) noexcept {
  return SdlRect{
      .x = static_cast<decltype(SdlRect::x)>(o.left),
      .y = static_cast<decltype(SdlRect::y)>(o.top),
      .w = static_cast<decltype(SdlRect::w)>(o.Width()),
      .h = static_cast<decltype(SdlRect::h)>(o.Height()),
  };
}

inline SDL_Texture *TexturePostInit(SDL_Texture &tex,
                                    SDL_Renderer * /*renderer*/) {
  SDL_SetTextureScaleMode(&tex, RenderState().texture_scale_mode);
  return &tex;
}

inline bool SetRenderTargetFor(const SDL_Renderer *renderer) {
  if (renderer == RenderState().software_renderer) {
    return SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);
  }
  if ((renderer == RenderState().primary_renderer) &&
      (RenderState().primary_texture != nullptr)) {
    return SDL_SetRenderTarget(RenderState().primary_renderer,
                               RenderState().primary_texture);
  }
  return true;
}

inline void SwitchActiveRenderer(SDL_Renderer *new_renderer) {
  for (auto &tex : RenderState().textures) {
    if (tex == nullptr) {
      continue;
    }
    const auto *renderer = SDL_GetRendererFromTexture(tex);
    if ((renderer != nullptr) && (renderer == RenderState().active_renderer)) {
      tex.Reset();
    }
  }
  SetRenderTargetFor(new_renderer);
  RenderState().active_renderer = new_renderer;
}

inline PixelPoint TextureSize(SDL_Texture *tex) {
  if (tex == nullptr) {
    return {.x = 0, .y = 0};
  }
  float w = 0;
  float h = 0;
  if (!SDL_GetTextureSize(tex, &w, &h)) {
    return {.x = 0, .y = 0};
  }
  return {
      .x = static_cast<int>(w),
      .y = static_cast<int>(h),
  };
}

inline bool UpdateTexture(SDL_Texture *tex, const Rect *subrect,
                          std::tuple<const uint8_t *, size_t> pixels) noexcept {
  const auto [buf, pitch] = pixels;
  if (pitch > std::numeric_limits<int>::max()) {
    logging::Critical(kLogCat, "Pitch of {} bytes does not fit into an integer",
                      pitch);
    return false;
  }
  if (subrect == nullptr) {
    return SDL_UpdateTexture(tex, nullptr, buf, static_cast<int>(pitch));
  }
  const auto rect = HelpRectTo<SDL_Rect>(*subrect);
  return SDL_UpdateTexture(tex, &rect, buf, static_cast<int>(pitch));
}

inline bool BlitTexture(SDL_Texture *tex, PixelPoint topleft, const Rect &src) {
  const auto rect_src = HelpRectTo<SDL_FRect>(src);
  const SDL_FRect rect_dst = {
      .x = static_cast<float>(topleft.x),
      .y = static_cast<float>(topleft.y),
      .w = static_cast<float>(rect_src.w),
      .h = static_cast<float>(rect_src.h),
  };
  return SDL_RenderTexture(RenderState().active_renderer, tex, &rect_src,
                           &rect_dst);
}

inline void DrawWithAlpha(auto func) {
  SDL_SetRenderDrawBlendMode(RenderState().active_renderer,
                             RenderState().alpha_mode);
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b,
                         RenderState().color.a);
  func();
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
  SDL_SetRenderDrawBlendMode(RenderState().active_renderer, SDL_BLENDMODE_NONE);
}

// Defined in graphics_sdl_pixels.cpp.
SDL_Texture *EnsureSoftwareTexture();

inline bool DestroySoftwareRenderer() {
  RenderState().software_renderer.Reset();
  RenderState().software_texture.Reset();
  return false;
}

// Pretty API version strings
// --------------------------

inline constexpr std::array<std::pair<std::string_view, std::string_view>, 5>
    kApiNiceNames = {{
        {"direct3d", "Direct3D 9"},
        {"direct3d11", "Direct3D 11"},
        {"direct3d12", "Direct3D 12"},
        {"software", "Software"},
        {"vulkan", "Vulkan"},
    }};
// --------------------------