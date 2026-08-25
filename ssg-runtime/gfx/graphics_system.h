///
/// The graphics system - owns the platform window, renderer, and text state.
///

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

#include "gfx/graphics.h"
#include "gfx/text/text_renderer.h"
#include "gfx/text/text_state.h"
#include "util/enum_array.h"
#include "util/sdl_resource.h"

namespace gfx {

// Pretty renderer API version strings (updated after renderer creation).
namespace api_versions {

struct Version {
  std::string_view name_sdl;
  const char *name_pretty = nullptr;
  void (*update)(Version &self) = nullptr;
  std::array<char, 64> buf{};
};

void UpdateGpu(Version &self);
void UpdateOpenGl(Version &self);

class VersionCatalog {
  static void SetName(Version &version, std::string_view name) {
    // Bounded copy: never overflow the fixed-size buffer, always NUL-terminate.
    const auto n = std::min(name.size(), version.buf.size() - 1);
    std::ranges::copy_n(name.begin(), n, version.buf.begin());
    version.buf[n] = '\0';
  }

public:
  VersionCatalog() noexcept {
    versions_[0] = {
        .name_sdl = "gpu", .name_pretty = nullptr, .update = UpdateGpu};
    SetName(versions_[0], "GPU");
    versions_[1] = {
        .name_sdl = "opengl", .name_pretty = "OpenGL", .update = UpdateOpenGl};
    SetName(versions_[1], "OpenGL ~2.1");
    versions_[2] = {.name_sdl = "opengles2",
                    .name_pretty = "OpenGL ES",
                    .update = UpdateOpenGl};
    SetName(versions_[2], "OpenGL ES ~2.0");
  }

  void Update(std::string_view driver_str) {
    auto version = std::ranges::find(versions_, driver_str, &Version::name_sdl);
    if (version == std::end(versions_)) {
      return;
    }
    version->update(*version);
  }

  [[nodiscard]] std::string_view Label(std::string_view api) const {
    for (const auto &version : versions_) {
      if (version.name_sdl == api) {
        return {version.buf.data()};
      }
    }
    return {};
  }

private:
  std::array<Version, 3> versions_;
};

} // namespace api_versions

// The SDL renderer, its textures, and the active-renderer switch. Member order
// matters for destruction: textures must die before the renderer that owns
// them.
struct RendererState {
  SDL_ScaleMode texture_scale_mode = SDL_SCALEMODE_NEAREST;
  util::SdlResource<SDL_Renderer, SDL_DestroyRenderer> primary_renderer;
  std::span<const SDL_PixelFormat> primary_formats;
  util::SdlResource<SDL_Texture, SDL_DestroyTexture> primary_texture;
  util::SdlResource<SDL_Renderer, SDL_DestroyRenderer> software_renderer;
  util::SdlResource<SDL_Surface, SDL_DestroySurface> software_surface;
  util::SdlResource<SDL_Texture, SDL_DestroyTexture> software_texture;
  util::SdlResource<SDL_Texture, SDL_DestroyTexture> text_atlas;
  util::EnumArray<util::SdlResource<SDL_Texture, SDL_DestroyTexture>, SurfaceId>
      textures{};
  SDL_Renderer *active_renderer = nullptr;
  api_versions::VersionCatalog versions;
  Rgba color = {.r = 0, .g = 0, .b = 0, .a = 0xFF};
  SDL_BlendMode alpha_mode = SDL_BLENDMODE_NONE;
};

struct WindowState {
  util::SdlResource<SDL_Window, SDL_DestroyWindow> window;
  std::optional<std::pair<int, int>> topleft_before_fullscreen;
};

// Policy state that lives outside the backend (frame rate limiting).
struct GraphicsState {
  int frame_rate_divisor = 0;
};

// Single owner of all persistent graphics, window, and text state. The public
// free functions in "gfx/graphics.h" and the backends operate on the active
// instance.
class GraphicsSystem {
public:
  RendererState renderer;
  WindowState window;
  GraphicsState policy;
  TextState text;
  TextRender cache;
};

GraphicsSystem &ActiveGraphics();

} // namespace gfx
