///
/// Graphics via SDL_Renderer
///

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <utility>

#include <SDL3/SDL_blendmode.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>

#include "geometry.h"
#include "graphics_sdl.h"

#include "gfx/core/constants.h"
#include "gfx/core/coords.h"
#include "gfx/core/pixelformat.h"
#include "gfx/graphics.h"
#include "gfx/graphics_system.h"
#include "gfx/image/format_bmp.h"
#include "gfx/image/screenshot.h"
#include "gfx/window/window_sdl.h"
#include "sys/log.h"
#include "util/enum_array.h"
#include "util/guard.h"
#include "util/sdl_resource.h"

using SdlColor = SDL_FColor;

constexpr auto kLogCat = logging::Channel::Graphics;

namespace {

gfx::RendererState &RenderState() { return gfx::ActiveGraphics().renderer; }

} // namespace

namespace {

SDL_Texture *EnsureSoftwareTexture();

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

// Helpers
// -------

template <typename SdlRect> SdlRect HelpRectTo(const Rect &o) noexcept {
  return SdlRect{
      .x = static_cast<decltype(SdlRect::x)>(o.left),
      .y = static_cast<decltype(SdlRect::y)>(o.top),
      .w = static_cast<decltype(SdlRect::w)>(o.Width()),
      .h = static_cast<decltype(SdlRect::h)>(o.Height()),
  };
}

SDL_Texture *TexturePostInit(SDL_Texture &tex, SDL_Renderer * /*renderer*/) {
  SDL_SetTextureScaleMode(&tex, RenderState().texture_scale_mode);
  return &tex;
}

bool SetRenderTargetFor(const SDL_Renderer *renderer) {
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

void SwitchActiveRenderer(SDL_Renderer *new_renderer) {
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

std::optional<WindowFullscreenState>
HelpSwitchFullscreen(const WindowFullscreenState &previous,
                     const WindowFullscreenState &requested) {
  auto *window = SdlWindow();

  if (!previous.enabled && requested.enabled) {
    SdlWindowRememberPosition(SdlWindowPosition(window));
  }

  const auto actual = SdlSetFullscreen(window, requested);
  if (!actual) {
    return std::nullopt;
  }

  // If we come out of fullscreen mode, recenter the window.
  if (previous.enabled && !requested.enabled) {
    const auto display_i = SdlDisplayForWindow();
    const auto center = SDL_WINDOWPOS_CENTERED_DISPLAY(display_i);
    SDL_SetWindowPosition(window, center, center);
  }
  return actual;
}
// -------

// Pretty API version strings
// --------------------------

constexpr std::array<std::pair<std::string_view, std::string_view>, 5>
    kApiNiceNames = {{
        {"direct3d", "Direct3D 9"},
        {"direct3d11", "Direct3D 11"},
        {"direct3d12", "Direct3D 12"},
        {"software", "Software"},
        {"vulkan", "Vulkan"},
    }};
// --------------------------

} // namespace

namespace gfx::api_versions {
void UpdateGpu(Version &self) {
  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  auto *gpu_device = static_cast<SDL_GPUDevice *>(SDL_GetPointerProperty(
      props, SDL_PROP_RENDERER_GPU_DEVICE_POINTER, nullptr));

  std::string_view device_name = SDL_GetGPUDeviceDriver(gpu_device);
  for (const auto &nice : kApiNiceNames) {
    if (nice.first == device_name) {
      device_name = nice.second;
      break;
    }
  }
  const auto *via_name = (device_name.empty() ? "?" : device_name.data());
  (void)std::snprintf(self.buf.data(), self.buf.size(), "GPU (%s)", via_name);
}

void UpdateOpenGl(Version &self) {
  int major = 0;
  int minor = 0;
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

  (void)std::snprintf(self.buf.data(), self.buf.size(), "%s %d.%d",
                      self.name_pretty, major, minor);
}
} // namespace gfx::api_versions

/// Enumeration and pre-initialization queries
/// ------------------------------------------

int GraphicsRenderDriverCount() { return SDL_GetNumRenderDrivers(); }

int GraphicsRenderDriverId(std::string_view driver) {
  for (const auto id : std::views::iota(0, SDL_GetNumRenderDrivers())) {
    if (GraphicsRenderDriverName(id) == driver) {
      return id;
    }
  }
  return -1;
}

std::string_view GraphicsRenderDriverName(int id) {
  const auto *name = SDL_GetRenderDriver(id);
  return name != nullptr ? std::string_view{name} : std::string_view{};
}

std::string_view GraphicsRenderDriverLabel(std::string_view api) {
  for (const auto &nice : kApiNiceNames) {
    if (nice.first == api) {
      return nice.second;
    }
  }
  const auto label = RenderState().versions.Label(api);
  if (!label.empty()) {
    return label;
  }
  return api;
}
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

namespace {

bool DestroySoftwareRenderer() {
  RenderState().software_renderer.Reset();
  RenderState().software_texture.Reset();
  return false;
}

std::nullopt_t PrimaryCleanup() {
  for (auto &tex : RenderState().textures) {
    tex.Reset();
  }
  RenderState().software_texture.Reset();
  RenderState().text_atlas.Reset();
  RenderState().primary_texture.Reset();
  RenderState().primary_renderer.Reset();
  RenderState().active_renderer = nullptr;

  // On my system, switching from any Direct3D version to OpenGL sometimes
  // breaks rendering and freezes the window on the last frame rendered by
  // Direct3D. So it makes sense to unconditionally destroy and recreate the
  // window when switching APIs. (Also feels more effective, in a way.)
  SdlWindowCleanup();

  return std::nullopt;
}

// Returns the new `SCALE_GEOMETRY` flag.
bool PrimarySetScale(bool geometry, const PixelPoint &scaled_res) {
  const auto set_geometry = [] {
    RenderState().primary_texture.Reset();
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer,
                                     kGameResolution.x, kGameResolution.y,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);
    return true;
  };

  // Update texture filters
  // ----------------------
  if (((scaled_res.x % kGameResolution.x) != 0) ||
      ((scaled_res.y % kGameResolution.y) != 0)) {
    RenderState().texture_scale_mode = SDL_SCALEMODE_LINEAR;
  } else {
    RenderState().texture_scale_mode = SDL_SCALEMODE_NEAREST;
  }
  for (auto &tex : RenderState().textures) {
    if (tex != nullptr) {
      SDL_SetTextureScaleMode(tex, RenderState().texture_scale_mode);
    }
  }
  if (RenderState().software_texture != nullptr) {
    SDL_SetTextureScaleMode(RenderState().software_texture,
                            RenderState().texture_scale_mode);
  }
  if (RenderState().primary_texture != nullptr) {
    SDL_SetTextureScaleMode(RenderState().primary_texture,
                            RenderState().texture_scale_mode);
  }
  // ----------------------

  if (geometry || (scaled_res == kGameResolution)) {
    set_geometry();
    return geometry; // Don't unset the user's choice on 1× scaling!
  }

  // Prepare the primary renderer for blitting the primary texture:
  // • Ensure the correct logical size
  // • Stop clipping on the primary renderer!!! Its clipping region is going
  //   to apply to the primary texture blit going forward!!!
  SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);
  SDL_SetRenderClipRect(RenderState().primary_renderer, nullptr);
  SDL_SetRenderLogicalPresentation(RenderState().primary_renderer, scaled_res.x,
                                   scaled_res.y,
                                   SDL_LOGICAL_PRESENTATION_DISABLED);

  if (RenderState().primary_texture == nullptr) {
    if (RenderState().software_surface == nullptr) {
      logging::Critical(
          kLogCat,
          "Cannot create the native resolution texture without a software "
          "surface");
      return set_geometry();
    }
    const auto format = RenderState().software_surface->format;
    const auto &res = kGameResolution;
    RenderState().primary_texture =
        SDL_CreateTexture(RenderState().primary_renderer, format,
                          SDL_TEXTUREACCESS_TARGET, res.x, res.y);
    if (RenderState().primary_texture == nullptr) {
      logging::SdlError(kLogCat, "Error creating native resolution texture");
      return set_geometry();
    }
    SDL_SetTextureScaleMode(RenderState().primary_texture,
                            RenderState().texture_scale_mode);
  }

  // We might be software-rendering.
  if (!SetRenderTargetFor(RenderState().active_renderer)) {
    logging::SdlError(kLogCat, "Error setting texture as render target");
    return set_geometry();
  }
  return geometry;
}

// Re-centers the window to remain fully on-screen after changing the
// windowed-mode scale factor
PixelPoint RepositionAfterScale(const PixelPoint &topleft_prev,
                                const PixelPoint &res_prev,
                                const PixelPoint &res_new) {
  auto *window = SdlWindow();
  int border_left{};
  int border_top{};
  int border_right{};
  int border_bottom{};
  SDL_GetWindowBordersSize(window, &border_top, &border_left, &border_bottom,
                           &border_right);

  const auto display_i = SDL_GetDisplayForWindow(window);
  if (display_i == 0) {
    return topleft_prev;
  }
  SDL_Rect display_r{};
  if (!SDL_GetDisplayUsableBounds(display_i, &display_r)) {
    return topleft_prev;
  }
  display_r.x += border_left;
  display_r.y += border_top;
  display_r.w -= (border_left + border_right);
  display_r.h -= (border_top + border_bottom);

  // The window might have been moved to a display with a resolution
  // smaller than [res_new].
  const auto max_left =
      std::max(display_r.x, (display_r.x + display_r.w - res_new.x));
  const auto max_top =
      std::max(display_r.y, (display_r.y + display_r.h - res_new.y));

  auto topleft = ((topleft_prev + (res_prev / 2)) - (res_new / 2));
  topleft.x = std::clamp(topleft.x, display_r.x, max_left);
  topleft.y = std::clamp(topleft.y, display_r.y, max_top);
  SDL_SetWindowPosition(window, topleft.x, topleft.y);
  return topleft;
}

void PrimarySetBorderlessFullscreenFit(GraphicsParams params,
                                       const PixelPoint & /*scaled_res*/) {
  using Fit = GraphicsFullscreenFit;

  auto *target = SDL_GetRenderTarget(RenderState().primary_renderer);
  SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);

  if (params.fullscreen && !params.exclusive_fullscreen) {
    constexpr auto kModes = [] {
      util::EnumArray<SDL_RendererLogicalPresentation, Fit> ret;
      ret[Fit::Integer] = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
      ret[Fit::Aspect] = SDL_LOGICAL_PRESENTATION_LETTERBOX;
      ret[Fit::Stretch] = SDL_LOGICAL_PRESENTATION_STRETCH;
      return ret;
    }();
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer,
                                     kGameResolution.x, kGameResolution.y,
                                     kModes[params.fullscreen_fit]);
  }
  SDL_SetRenderTarget(RenderState().primary_renderer, target);
}

std::optional<GraphicsInitResult> PrimaryInitFull(GraphicsParams params) {
  const auto maybe_params = SdlWindowCreate(params);
  if (!maybe_params) {
    return std::nullopt;
  }
  params = maybe_params.value();

  const auto *driver = SDL_GetRenderDriver(params.render_driver);
  RenderState().primary_renderer = SDL_CreateRenderer(SdlWindow(), driver);
  if (RenderState().primary_renderer == nullptr) {
    const auto driver_str = SdlRenderDriverName(params.render_driver);
    const auto label = GraphicsRenderDriverLabel(driver_str);
    const auto *api = label.data();
    logging::Critical(kLogCat, "Error creating {} renderer: {}", api,
                      SDL_GetError());
    return PrimaryCleanup();
  }
  const auto driver_str = GraphicsActiveRenderDriver();
  logging::Info(kLogCat, "Using SDL renderer: {}",
                GraphicsRenderDriverLabel(driver_str));

  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  const auto *formats_start =
      static_cast<const SDL_PixelFormat *>(SDL_GetPointerProperty(
          props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, nullptr));
  const auto *formats_end = formats_start;
  while (*formats_end != SDL_PIXELFORMAT_UNKNOWN) {
    formats_end++;
  }
  RenderState().primary_formats = {
      formats_start,
      static_cast<size_t>(formats_end - formats_start),
  };

  // Verify the renderer supports BGRA8888 (SDL ARGB8888).
  constexpr auto sdl_format = SDL_PIXELFORMAT_ARGB8888;
  if (!std::ranges::contains(RenderState().primary_formats, sdl_format)) {
    const auto label = GraphicsRenderDriverLabel(driver_str);
    logging::Critical(
        kLogCat,
        "The \"{}\" renderer does not support the BGRA8888 pixel format "
        "required for software rendering",
        label);
    return PrimaryCleanup();
  }

  SetRenderTargetFor(RenderState().primary_renderer);
  RenderState().versions.Update(driver_str);

  // Ensure that the software surface uses the preferred format
  if ((RenderState().software_surface == nullptr) ||
      (RenderState().software_surface->format != sdl_format)) {
    RenderState().software_surface =
        SDL_CreateSurface(kGameResolution.x, kGameResolution.y, sdl_format);
    if (RenderState().software_surface == nullptr) {
      logging::SdlError(kLogCat,
                        "Error creating surface for software rendering");
      return PrimaryCleanup();
    }
  }

  const auto res_new =
      params.ScaledRes(SdlGraphicsDisplaySize(params.fullscreen));
  params.scale_geometry = PrimarySetScale(params.scale_geometry, res_new);
  PrimarySetBorderlessFullscreenFit(params, res_new);

  return GraphicsInitResult{.live = params, .reload_surfaces = true};
}

} // namespace

std::optional<GraphicsInitResult>
SdlGraphicsInit(std::optional<const GraphicsParams> maybe_prev,
                GraphicsParams params) {
  const auto reinit_full = [](const GraphicsParams &params) {
    PrimaryCleanup();
    return PrimaryInitFull(params);
  };

  const WindowFullscreenState requested_fullscreen = {
      .enabled = params.fullscreen,
      .exclusive = params.exclusive_fullscreen,
  };

  // This is the only place that applies to both a full init and a partial
  // update later...
  if (requested_fullscreen.enabled && requested_fullscreen.exclusive) {
    SDL_HideCursor();
  } else {
    SDL_ShowCursor();
  }

  if (!maybe_prev) {
    return PrimaryInitFull(params);
  }
  const auto &prev = maybe_prev.value();
  const WindowFullscreenState previous_fullscreen = {
      .enabled = prev.fullscreen,
      .exclusive = prev.exclusive_fullscreen,
  };

  // API changes need a complete reinit.
  if (prev.render_driver != params.render_driver) {
    return reinit_full(params);
  }

  // As do a few things when switching to exclusive fullscreen.
  if (requested_fullscreen.enabled && requested_fullscreen.exclusive) {
    // The Direct3D renderer can only launch into exclusive fullscreen on
    // the same display the window was spawned on, so let's just throw it
    // away.
    const auto name = SdlRenderDriverName(params.render_driver);
    if (!previous_fullscreen.enabled && (name == "direct3d")) {
      return reinit_full(params);
    }
  }

  // The following parameters can be changed on the fly, but we don't want to
  // reflect modifications of any parameters we don't care about.
  GraphicsInitResult ret = {.live = prev, .reload_surfaces = false};

  // Apply fullscreen changes first, as exclusive fullscreen affects the
  // display size calculated by ScaledRes().
  if (previous_fullscreen.enabled != requested_fullscreen.enabled ||
      previous_fullscreen.exclusive != requested_fullscreen.exclusive) {
    const auto actual =
        HelpSwitchFullscreen(previous_fullscreen, requested_fullscreen);
    if (actual) {

      // If we clipped on the raw renderer, the clipping rectangle won't
      // match the current resolution anymore.
      SDL_SetRenderClipRect(RenderState().primary_renderer, nullptr);

      ret.live.fullscreen = actual->enabled;
      ret.live.exclusive_fullscreen = actual->exclusive;
    }
  }

  auto *window = SdlWindow();
  PixelPoint res_prev{};
  SDL_GetWindowSize(window, &res_prev.x, &res_prev.y);

  const auto res_new =
      params.ScaledRes(SdlGraphicsDisplaySize(params.fullscreen));
  const bool res_changed = (res_prev != res_new);
  if (res_changed && !requested_fullscreen.enabled) {
    PixelPoint topleft{};
    SDL_GetWindowPosition(window, &topleft.x, &topleft.y);
    SDL_SetWindowSize(window, res_new.x, res_new.y);

    // Necessary for the X11 backend.
    SDL_SyncWindow(window);

    topleft = RepositionAfterScale(topleft, res_prev, res_new);
    ret.live.window_left = topleft.x;
    ret.live.window_top = topleft.y;
  } else {
    ret.live.window_left = params.window_left;
    ret.live.window_top = params.window_top;
  }

  // Should always be applied unconditionally so that the user can change
  // from the maximum scale value to 0, both of which result in the same
  // scaled resolution.
  ret.live.window_scale_quarters = params.window_scale_quarters;

  ret.live.scale_geometry = PrimarySetScale(params.scale_geometry, res_new);

  PrimarySetBorderlessFullscreenFit(params, res_new);
  ret.live.fullscreen_fit = params.fullscreen_fit;

  return ret;
}

void GraphicsCleanup() {
  PrimaryCleanup();
  DestroySoftwareRenderer();
  RenderState().software_surface.Reset();
}
/// --------------------------

/// General
/// -------

void GraphicsClear(Rgb col) {
  SDL_SetRenderDrawColor(RenderState().active_renderer, col.r, col.g, col.b, 0xFF);
  SDL_RenderClear(RenderState().active_renderer);
}

void GraphicsSetClip(const Rect &rect) {
  if ((RenderState().active_renderer) == nullptr) {
    return;
  }
  const auto sdl_rect = HelpRectTo<SDL_Rect>(rect);
  SDL_SetRenderClipRect(RenderState().active_renderer, &sdl_rect);
}

std::string_view GraphicsActiveRenderDriver() {
  // More efficient than the hash table insertion done by
  // SDL_GetRendererName().
  if (RenderState().primary_renderer == nullptr) {
    logging::Critical(kLogCat,
                      "Cannot query the graphics API before initialization");
    return {};
  }
  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  const auto *name =
      SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, nullptr);
  return name != nullptr ? std::string_view{name} : std::string_view{};
}

namespace {

void TakeScreenshot() {
  SDL_FlushRenderer(RenderState().active_renderer);

  if (RenderState().software_renderer != nullptr) {
    // Software rendering is the ideal case for screenshots, because we
    // already have a system-memory surface we can save.
    image::ScreenshotSave(RenderState().software_surface);
    return;
  }

  SDL_Surface *src =
      SDL_RenderReadPixels(RenderState().primary_renderer, nullptr);
  if (src == nullptr) {
    logging::SdlError(kLogCat, "Error taking screenshot");
    return;
  }
  auto src_guard = util::MakeGuard(src, SDL_DestroySurface);
  image::ScreenshotSave(src);
}

} // namespace

void SdlGraphicsFlip(bool take_screenshot) {
  if (take_screenshot) {
    TakeScreenshot();
  }
  if (RenderState().software_renderer != nullptr) {
    SDL_FlushRenderer(RenderState().software_renderer);
    const bool must_lock = SDL_MUSTLOCK(RenderState().software_surface);
    if (must_lock && !SDL_LockSurface(RenderState().software_surface)) {
      logging::SdlError(kLogCat, "Error locking software backbuffer");
      return;
    }
    auto unlock_guard = util::MakeGuard([&] {
      if (must_lock) {
        SDL_UnlockSurface(RenderState().software_surface);
      }
    });
    auto *tex = EnsureSoftwareTexture();
    if (tex == nullptr) {
      return;
    }
    SDL_UpdateTexture(tex, nullptr, RenderState().software_surface->pixels,
                      RenderState().software_surface->pitch);
    SDL_RenderTexture(RenderState().primary_renderer,
                      RenderState().software_texture, nullptr, nullptr);
    SDL_RenderPresent(RenderState().primary_renderer);
  } else if (RenderState().primary_texture != nullptr) {
    SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);

    // In borderless fullscreen mode, the scaled texture may not cover the
    // entire window. Technically, we only need to do this once for every
    // backbuffer after switching the fullscreen fit, but:
    // 1) SDL has no way of querying the length of the swapchain, and
    // 2) you are supposed to do this on every frame anyway, as a lot of
    //    GPUs can use clearing as a performance hint.
    // Let's measure the performance impact on windowed mode some other
    // time...
    GraphicsClear();

    SDL_RenderTexture(RenderState().primary_renderer,
                      RenderState().primary_texture, nullptr, nullptr);

    // SDL_RenderPresent() is not allowed to be called when rendering to a
    // texture, and fails as of SDL 3.2.8:
    //
    // 	https://github.com/libsdl-org/SDL/issues/12432
    SDL_RenderPresent(RenderState().primary_renderer);
    SDL_SetRenderTarget(RenderState().primary_renderer,
                        RenderState().primary_texture);
  } else {
    SDL_RenderPresent(RenderState().primary_renderer);
  }
}
/// -------

/// Surfaces
/// --------

namespace {

bool CreateTextureWithFormat(SurfaceId sid, SDL_PixelFormat fmt,
                             const PixelPoint &size) {
  auto &tex = RenderState().textures[sid];
  tex.Reset();

  tex = SDL_CreateTexture(RenderState().active_renderer, fmt,
                          SDL_TEXTUREACCESS_STREAMING, size.x, size.y);
  if (tex == nullptr) {
    logging::SdlError(kLogCat, "Error creating blank texture");
    return false;
  }
  TexturePostInit(*tex, RenderState().active_renderer);
  if (!SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)) {
    logging::SdlError(kLogCat, "Error enabling alpha blending for texture");
    tex.Reset();
    return false;
  }
  return true;
}

} // namespace

bool GraphicsSurfaceCreateUninitialized(SurfaceId sid, const PixelPoint &size) {
  return CreateTextureWithFormat(sid, SDL_PIXELFORMAT_ARGB8888, size);
}

bool GraphicsSurfaceLoad(SurfaceId sid, BmpOwned bmp) {
  auto &tex = RenderState().textures[sid];
  tex.Reset();

  auto *rwops = SDL_IOFromMem(bmp.buffer.data(), bmp.buffer.size());
  if (rwops == nullptr) {
    logging::SdlError(kLogCat, "Error opening .BMP memory stream");
    return false;
  }
  auto *surf = SDL_LoadBMP_IO(rwops, true);
  if (surf == nullptr) {
    logging::SdlError(kLogCat, "Error decoding .BMP surface");
    return false;
  }
  auto surf_guard = util::MakeGuard(surf, SDL_DestroySurface);

  if (surf->format == SDL_PIXELFORMAT_INDEX8) {
    // The transparent pixel is in the top-left corner.
    const bool must_lock = SDL_MUSTLOCK(surf);
    if (must_lock && !SDL_LockSurface(surf)) {
      logging::SdlError(kLogCat, "Error locking indexed .BMP surface");
      return false;
    }
    const auto key = static_cast<uint8_t *>(surf->pixels)[0];
    if (must_lock) {
      SDL_UnlockSurface(surf);
    }
    SDL_SetSurfaceColorKey(surf, true, key);
  }

  tex = SDL_CreateTextureFromSurface(RenderState().active_renderer, surf);
  if (tex == nullptr) {
    logging::SdlError(kLogCat, "Error loading .BMP as texture");
    return false;
  }
  TexturePostInit(*tex, RenderState().active_renderer);
  return true;
}

namespace {

PixelPoint TextureSize(SDL_Texture *tex) {
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

bool UpdateTexture(SDL_Texture *tex, const Rect *subrect,
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

bool BlitTexture(SDL_Texture *tex, PixelPoint topleft, const Rect &src) {
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

} // namespace

bool GraphicsSurfaceUpdate(
    SurfaceId sid, const Rect *subrect,
    std::tuple<const uint8_t *, size_t> pixels) noexcept {
  return UpdateTexture(RenderState().textures[sid].get(), subrect, pixels);
}

PixelPoint GraphicsSurfaceSize(SurfaceId sid) {
  return TextureSize(RenderState().textures[sid].get());
}

bool GraphicsSurfaceBlit(PixelPoint topleft, SurfaceId sid, const Rect &src) {
  return BlitTexture(RenderState().textures[sid].get(), topleft, src);
}

PixelPoint SdlTextTextureSize() { return TextureSize(RenderState().text_atlas); }

bool SdlTextTexturePrepare(PixelPoint size) {
  auto &tex = RenderState().text_atlas;
  tex.Reset();
  tex = SDL_CreateTexture(RenderState().active_renderer,
                          SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                          size.x, size.y);
  if (tex == nullptr) {
    logging::SdlError(kLogCat, "Error creating text atlas texture");
    return false;
  }
  TexturePostInit(*tex, RenderState().active_renderer);
  if (!SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)) {
    logging::SdlError(kLogCat, "Error enabling alpha blending for text texture");
    tex.Reset();
    return false;
  }
  return true;
}

bool SdlTextTextureUpdate(const Rect *subrect,
                          std::tuple<const uint8_t *, size_t> pixels) noexcept {
  return UpdateTexture(RenderState().text_atlas, subrect, pixels);
}

bool SdlTextTextureBlit(PixelPoint topleft, const Rect &src) {
  return BlitTexture(RenderState().text_atlas, topleft, src);
}

void GraphicsSurfaceBlitOpaque(PixelPoint topleft, SurfaceId sid,
                               const Rect &src) {
  auto *tex = RenderState().textures[sid].get();
  SDL_BlendMode prev{};
  SDL_GetTextureBlendMode(tex, &prev);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
  GraphicsSurfaceBlit(topleft, sid, src);
  SDL_SetTextureBlendMode(tex, prev);
}

void GraphicsSurfaceSetColorMod(SurfaceId sid, uint8_t r, uint8_t g,
                                uint8_t b) {
  SDL_SetTextureColorMod(RenderState().textures[sid], r, g, b);
}

/// Geometry
/// --------

namespace {

void DrawGeometry(TrianglePrimitive tp, VertexXySpan<> xys,
                  VertexRgbaSpan<> colors) {
#pragma warning(suppress : 26494) // type.5
  std::array<SDL_FPoint, kMaxTriangles> sdl_vertices{};
  std::array<SdlColor, kMaxTriangles> sdl_colors{};

  const auto vertex_count = xys.size();
  if (vertex_count < 3 || vertex_count > std::size(sdl_vertices)) {
    logging::Critical(kLogCat,
                      "Invalid vertex count for a triangle primitive: {}",
                      vertex_count);
    return;
  }
  const auto indices = kTriangleIndices[tp];
  const auto index_count = TriangleIndexCount(vertex_count);
  if (index_count > indices.size()) {
    logging::Critical(kLogCat,
                      "Invalid index count for a triangle primitive: {}",
                      index_count);
    return;
  }

  if (colors.size() != 1 && colors.size() != vertex_count) {
    logging::Critical(kLogCat,
                      "Triangle color count {} does not match vertex count {}",
                      colors.size(), vertex_count);
    return;
  }
  std::ranges::transform(colors, sdl_colors.begin(), [](const auto &color) {
    return SdlColor{.r = color.r, .g = color.g, .b = color.b, .a = color.a};
  });

  // Work around SDL's weird -0.5f offset...
  float offset_x = NAN;
  float offset_y = NAN;
  SDL_GetRenderScale(RenderState().active_renderer, &offset_x, &offset_y);
  offset_x = (1.0F / (2.0F * offset_x));
  offset_y = (1.0F / (2.0F * offset_y));
  auto *sdl = sdl_vertices.data();
  for (const auto &game : xys) {
    *(sdl++) = {.x = (game.x + offset_x), .y = (game.y + offset_y)};
  }

  SDL_RenderGeometryRaw(
      RenderState().active_renderer, nullptr, &sdl_vertices[0].x, sizeof(SDL_FPoint),
      sdl_colors.data(), ((colors.size() == 1) ? 0 : sizeof(SdlColor)), nullptr,
      0, vertex_count, indices.data(), index_count, sizeof(IndexType));
}

void DrawWithAlpha(auto func) {
  SDL_SetRenderDrawBlendMode(RenderState().active_renderer, RenderState().alpha_mode);
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b,
                         RenderState().color.a);
  func();
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
  SDL_SetRenderDrawBlendMode(RenderState().active_renderer, SDL_BLENDMODE_NONE);
}

} // namespace

void geometry::SetColor(Rgb216 col) {
  const auto rgb = col.ToRgb();
  RenderState().color.r = rgb.r;
  RenderState().color.g = rgb.g;
  RenderState().color.b = rgb.b;
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
}

void geometry::SetAlphaNorm(uint8_t a) {
  RenderState().color.a = a;
  RenderState().alpha_mode = SDL_BLENDMODE_BLEND;
}

void geometry::SetAlphaOne() {
  RenderState().color.a = 0xFF;
  RenderState().alpha_mode = SDL_BLENDMODE_ADD;
}

void geometry::DrawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderLine(RenderState().active_renderer, x1, y1, x2, y2);
}

void geometry::DrawBox(int x1, int y1, int x2, int y2) {
  const SDL_FRect rect = {
      .x = static_cast<float>(x1),
      .y = static_cast<float>(y1),
      .w = static_cast<float>(x2 - x1),
      .h = static_cast<float>(y2 - y1),
  };
  SDL_RenderFillRect(RenderState().active_renderer, &rect);
}

void geometry::DrawBoxA(int x1, int y1, int x2, int y2) {
  DrawWithAlpha([&] { DrawBox(x1, y1, x2, y2); });
}

void geometry::DrawLineStrip(VertexXySpan<> xys) {
  if (xys.size() > kMaxTriangles) {
    logging::Critical(kLogCat, "Too many points for a line strip: {}",
                      xys.size());
    return;
  }
  std::array<SDL_FPoint, kMaxTriangles> points{};
  std::ranges::transform(xys, points.begin(), [](const auto &point) {
    return SDL_FPoint{.x = point.x, .y = point.y};
  });
  SDL_RenderLines(RenderState().active_renderer, points.data(), xys.size());
}

void geometry::DrawTriangles(TrianglePrimitive tp, VertexXySpan<> xys,
                             VertexRgbaSpan<> colors) {
  if (colors.empty()) {
    const VertexRgba single = {RenderState().color.r, RenderState().color.g,
                               RenderState().color.b, 0xFF};
    DrawGeometry(tp, xys, std::span(&single, 1));
  } else {
    DrawGeometry(tp, xys, colors);
  }
}

void geometry::DrawTrianglesA(TrianglePrimitive tp, VertexXySpan<> xys,
                              VertexRgbaSpan<> colors) {
  DrawWithAlpha([&] {
    if (colors.empty()) {
      const VertexRgba single = {RenderState().color.r, RenderState().color.g,
                                 RenderState().color.b, RenderState().color.a};
      DrawGeometry(tp, xys, std::span(&single, 1));
    } else {
      DrawGeometry(tp, xys, colors);
    }
  });
}

void geometry::DrawGrdLineEx(int x, int y1, Rgb c1, int y2, Rgb c2) {
  const auto c1a = c1.WithAlpha(0xFF);
  const auto c2a = c2.WithAlpha(0xFF);
  const std::array<VertexXy, 4> xys = {
      VertexXy{static_cast<float>(x + 0), static_cast<float>(y1)},
      VertexXy{static_cast<float>(x + 0), static_cast<float>(y2)},
      VertexXy{static_cast<float>(x + 1), static_cast<float>(y1)},
      VertexXy{static_cast<float>(x + 1), static_cast<float>(y2)},
  };
  const std::array<VertexRgba, 4> colors = {c1a, c2a, c1a, c2a};
  DrawGeometry(TrianglePrimitive::Strip, xys, colors);
}

/// --------

/// Software rendering with pixel access
/// ------------------------------------

namespace {

SDL_Texture *EnsureSoftwareTexture() {
  if (RenderState().software_texture != nullptr) {
    return RenderState().software_texture;
  }
  RenderState().software_texture = SDL_CreateTexture(
      RenderState().primary_renderer, RenderState().software_surface->format,
      SDL_TEXTUREACCESS_STREAMING, RenderState().software_surface->w,
      RenderState().software_surface->h);
  if (RenderState().software_texture == nullptr) {
    logging::SdlError(kLogCat, "Error creating software rendering texture");
    DestroySoftwareRenderer();
    return nullptr;
  }
  TexturePostInit(*RenderState().software_texture,
                  RenderState().primary_renderer);
  return RenderState().software_texture;
}

} // namespace

bool GraphicsPixelAccessStart() {
  if (RenderState().software_renderer != nullptr) {
    return true;
  }
  RenderState().software_renderer =
      SDL_CreateSoftwareRenderer(RenderState().software_surface);
  if (RenderState().software_renderer == nullptr) {
    logging::SdlError(kLogCat, "Error creating software renderer");
    return DestroySoftwareRenderer();
  }
  SwitchActiveRenderer(RenderState().software_renderer.get());
  return (EnsureSoftwareTexture() != nullptr);
}

bool GraphicsPixelAccessEnd() {
  if (RenderState().software_renderer == nullptr) {
    return true;
  }
  SwitchActiveRenderer(RenderState().primary_renderer.get());
  DestroySoftwareRenderer();
  return true;
}

std::tuple<uint8_t *, size_t> GraphicsPixelAccessLock() {
  if (RenderState().software_renderer == nullptr ||
      RenderState().software_surface == nullptr) {
    logging::Critical(kLogCat,
                      "Pixel access requires an active software renderer");
    return {nullptr, 0};
  }
  // Necessary in SDL 3!
  SDL_FlushRenderer(RenderState().software_renderer);

  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    if (!SDL_LockSurface(RenderState().software_surface)) {
      logging::SdlError(kLogCat, "Error locking CPU backbuffer");
      return {nullptr, 0};
    }
  }
  auto *pixels = static_cast<uint8_t *>(RenderState().software_surface->pixels);
  return {pixels, RenderState().software_surface->pitch};
}

void GraphicsPixelAccessUnlock() {
  if (RenderState().software_surface == nullptr) {
    return;
  }
  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    SDL_UnlockSurface(RenderState().software_surface);
  }
}
/// ------------------------------------
