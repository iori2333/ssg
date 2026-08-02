///
/// Graphics via SDL_Renderer
///

#include "SDL3/SDL_blendmode.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_iostream.h"
#include "SDL3/SDL_pixels.h"
#include "SDL3/SDL_properties.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_surface.h"
#include "SDL3/SDL_video.h"
#include "gfx/coords.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/pixelformat.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <ranges>
#include <span>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_render.h>
#include <cmath>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

using SdlColor = SDL_FColor;

#include "format_bmp.h"
#include "window_backend.h"
#include "window_sdl.h"

#include "gfx/constants.h"
#include "platform/text_backend.h"
#include "sys/log.h"
#include "util/enum_array.h"
#include "util/guard.h"

constexpr auto kLogCat = logging::Channel::Graphics;

namespace {

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
    std::ranges::copy(name, version.buf.begin());
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

struct RendererState {
  SDL_ScaleMode texture_scale_mode = SDL_SCALEMODE_NEAREST;
  SDL_Renderer *primary_renderer = nullptr;
  std::span<const SDL_PixelFormat> primary_formats;
  SDL_Texture *primary_texture = nullptr;
  SDL_Renderer *software_renderer = nullptr;
  SDL_Surface *software_surface = nullptr;
  SDL_Texture *software_texture = nullptr;
  SDL_Renderer **renderer = &primary_renderer;
  util::EnumArray<SDL_Texture *, SurfaceId> textures{};
  api_versions::VersionCatalog versions;
  Rgba color = {.r = 0, .g = 0, .b = 0, .a = 0xFF};
  SDL_BlendMode alpha_mode = SDL_BLENDMODE_NONE;
};

RendererState &RenderState() {
  static RendererState state;
  return state;
}

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

template <typename Rect> Rect HelpRectTo(const PixelLtwh &o) noexcept {
  return Rect{
      .x = static_cast<decltype(Rect::x)>(o.left),
      .y = static_cast<decltype(Rect::y)>(o.top),
      .w = static_cast<decltype(Rect::w)>(o.w),
      .h = static_cast<decltype(Rect::h)>(o.h),
  };
}

template <typename Rect> Rect HelpRectTo(const PixelLtrb &o) {
  return Rect{
      .x = static_cast<decltype(Rect::x)>(o.left),
      .y = static_cast<decltype(Rect::y)>(o.top),
      .w = static_cast<decltype(Rect::w)>(o.right - o.left),
      .h = static_cast<decltype(Rect::h)>(o.bottom - o.top),
  };
}

std::span<const SDL_FPoint> HelpFPointsFrom(VertexXySpan<> sp) {
  using GT = decltype(sp)::value_type;
  static_assert(sizeof(SDL_FPoint) == sizeof(GT));
  static_assert(std::is_same_v<decltype(SDL_FPoint::x), decltype(GT::x)>);
  static_assert(std::is_same_v<decltype(SDL_FPoint::y), decltype(GT::y)>);
  return {reinterpret_cast<const SDL_FPoint *>(sp.data()), sp.size()};
}

std::span<const SdlColor> HelpColorsFrom(VertexRgbaSpan<> sp) {
  using GT = decltype(sp)::value_type;
  static_assert(sizeof(SdlColor) == sizeof(GT));
  static_assert(std::is_same_v<decltype(SdlColor::r), decltype(GT::r)>);
  static_assert(std::is_same_v<decltype(SdlColor::g), decltype(GT::g)>);
  static_assert(std::is_same_v<decltype(SdlColor::b), decltype(GT::b)>);
  static_assert(std::is_same_v<decltype(SdlColor::a), decltype(GT::a)>);
  return {reinterpret_cast<const SdlColor *>(sp.data()), sp.size()};
}

SDL_Texture *TexturePostInit(SDL_Texture &tex, SDL_Renderer * /*renderer*/) {
  SDL_SetTextureScaleMode(&tex, RenderState().texture_scale_mode);
  return &tex;
}

template <typename T> [[nodiscard]] T *SafeDestroy(void Destroy(T *), T *v) {
  if (v) {
    Destroy(v);
    v = nullptr;
  }
  return v;
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

void SwitchActiveRenderer(SDL_Renderer **new_renderer) {
  for (auto &tex : RenderState().textures) {
    if (tex == nullptr) {
      continue;
    }
    const auto *renderer = SDL_GetRendererFromTexture(tex);
    if ((tex != nullptr) && (renderer == *RenderState().renderer)) {
      tex = SafeDestroy(SDL_DestroyTexture, tex);
    }
  }
  SetRenderTargetFor(*new_renderer);
  RenderState().renderer = new_renderer;
}

std::optional<GraphicsFullscreenFlags>
HelpSwitchFullscreen(const GraphicsFullscreenFlags &fs_prev,
                     const GraphicsFullscreenFlags &fs_new) {
  auto *window = WindowBackendSDL();

  if (!fs_prev.fullscreen && fs_new.fullscreen) {
    WindowBackendRememberTopleft(HelpGetWindowPosition(window));
  }

  const auto fs_actual = HelpSetFullscreenMode(window, fs_new);
  if (!fs_actual) {
    return std::nullopt;
  }

  // If we come out of fullscreen mode, recenter the window.
  if (fs_prev.fullscreen && !fs_new.fullscreen) {
    const auto display_i = HelpGetDisplayForWindow();
    const auto center = SDL_WINDOWPOS_CENTERED_DISPLAY(display_i);
    SDL_SetWindowPosition(window, center, center);
  }
  return fs_actual;
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

namespace api_versions {
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
} // namespace api_versions
// --------------------------

} // namespace

/// Enumeration and pre-initialization queries
/// ------------------------------------------

bool GraphicsBackendEnum() {
  // Any SDL-specific initialization was already done as part of
  // SDL_Init(SDL_INIT_VIDEO).
  return true;
}

int8_t GraphicsBackendAPICount() { return SDL_GetNumRenderDrivers(); }

std::string_view GraphicsBackendAPILabel(std::string_view api) {
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

int GraphicsBackendAPIID(std::string_view api) {
  for (const auto i : std::views::iota(0, SDL_GetNumRenderDrivers())) {
    if (GraphicsBackendAPIString(i) == api) {
      return i;
    }
  }
  return -1;
}

std::string_view GraphicsBackendAPIString(int8_t id) {
  const auto *ret = SDL_GetRenderDriver(id);
  return ((ret != nullptr) ? ret : std::string_view{});
}

PixelSize GraphicsBackendDisplaySize(bool fullscreen) {
  SDL_Rect rect{};
  const auto display_i = HelpGetDisplayForWindow();
  if (fullscreen) {
    const auto *display_mode = SDL_GetDesktopDisplayMode(display_i);
    if (display_mode == nullptr) {
      logging::SdlError(kLogCat, "Error retrieving display size");
      return kGameResolution;
    }
    return {.w = display_mode->w, .h = display_mode->h};
  }

  if (!SDL_GetDisplayUsableBounds(display_i, &rect)) {
    logging::SdlError(kLogCat, "Error retrieving display size");
    return kGameResolution;
  }
  return {.w = rect.w, .h = rect.h};
}
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

namespace {

bool DestroySoftwareRenderer() {
  RenderState().software_renderer =
      SafeDestroy(SDL_DestroyRenderer, RenderState().software_renderer);
  RenderState().software_texture =
      SafeDestroy(SDL_DestroyTexture, RenderState().software_texture);
  return false;
}

std::nullopt_t PrimaryCleanup() {
  for (auto &tex : RenderState().textures) {
    tex = SafeDestroy(SDL_DestroyTexture, tex);
  }
  RenderState().software_texture =
      SafeDestroy(SDL_DestroyTexture, RenderState().software_texture);
  RenderState().primary_texture =
      SafeDestroy(SDL_DestroyTexture, RenderState().primary_texture);
  RenderState().primary_renderer =
      SafeDestroy(SDL_DestroyRenderer, RenderState().primary_renderer);

  // On my system, switching from any Direct3D version to OpenGL sometimes
  // breaks rendering and freezes the window on the last frame rendered by
  // Direct3D. So it makes sense to unconditionally destroy and recreate the
  // window when switching APIs. (Also feels more effective, in a way.)
  WindowBackendCleanup();

  return std::nullopt;
}

// Returns the new `SCALE_GEOMETRY` flag.
bool PrimarySetScale(bool geometry, const WindowSize &scaled_res) {
  const auto set_geometry = [] {
    RenderState().primary_texture =
        SafeDestroy(SDL_DestroyTexture, RenderState().primary_texture);
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer,
                                     kGameResolution.w, kGameResolution.h,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);
    return true;
  };

  // Update texture filters
  // ----------------------
  if (((scaled_res.w % kGameResolution.w) != 0) ||
      ((scaled_res.h % kGameResolution.h) != 0)) {
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

  assert(!geometry);

  // Prepare the primary renderer for blitting the primary texture:
  // • Ensure the correct logical size
  // • Stop clipping on the primary renderer!!! Its clipping region is going
  //   to apply to the primary texture blit going forward!!!
  SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);
  SDL_SetRenderClipRect(RenderState().primary_renderer, nullptr);
  SDL_SetRenderLogicalPresentation(RenderState().primary_renderer, scaled_res.w,
                                   scaled_res.h,
                                   SDL_LOGICAL_PRESENTATION_DISABLED);

  if (RenderState().primary_texture == nullptr) {
    assert(RenderState().software_surface);
    const auto format = RenderState().software_surface->format;
    const auto &res = kGameResolution;
    RenderState().primary_texture =
        SDL_CreateTexture(RenderState().primary_renderer, format,
                          SDL_TEXTUREACCESS_TARGET, res.w, res.h);
    if (RenderState().primary_texture == nullptr) {
      logging::SdlError(kLogCat, "Error creating native resolution texture");
      return set_geometry();
    }
    SDL_SetTextureScaleMode(RenderState().primary_texture,
                            RenderState().texture_scale_mode);
  }

  // We might be software-rendering.
  if (!SetRenderTargetFor(*RenderState().renderer)) {
    logging::SdlError(kLogCat, "Error setting texture as render target");
    return set_geometry();
  }
  return geometry;
}

// Re-centers the window to remain fully on-screen after changing the
// windowed-mode scale factor
PixelPoint RepositionAfterScale(const PixelPoint &topleft_prev,
                                const WindowSize &res_prev,
                                const WindowSize &res_new) {
  auto *window = WindowBackendSDL();
  PixelCoord border_left{};
  PixelCoord border_top{};
  PixelCoord border_right{};
  PixelCoord border_bottom{};
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
      std::max(display_r.x, (display_r.x + display_r.w - res_new.w));
  const auto max_top =
      std::max(display_r.y, (display_r.y + display_r.h - res_new.h));

  auto topleft = ((topleft_prev + (res_prev / 2)) - (res_new / 2));
  topleft.x = std::clamp(topleft.x, display_r.x, max_left);
  topleft.y = std::clamp(topleft.y, display_r.y, max_top);
  SDL_SetWindowPosition(window, topleft.x, topleft.y);
  return topleft;
}

void PrimarySetBorderlessFullscreenFit(GraphicsParams params,
                                       const WindowSize & /*scaled_res*/) {
  using Fit = GraphicsFullscreenFit;
  const auto fs = params.FullscreenFlags();

  auto *target = SDL_GetRenderTarget(RenderState().primary_renderer);
  SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);

  if (fs.fullscreen && !fs.exclusive) {
    constexpr auto kModes = [] {
      util::EnumArray<SDL_RendererLogicalPresentation, Fit> ret;
      ret[Fit::Integer] = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
      ret[Fit::Aspect] = SDL_LOGICAL_PRESENTATION_LETTERBOX;
      ret[Fit::Stretch] = SDL_LOGICAL_PRESENTATION_STRETCH;
      return ret;
    }();
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer,
                                     kGameResolution.w, kGameResolution.h,
                                     kModes[fs.fit]);
  }
  SDL_SetRenderTarget(RenderState().primary_renderer, target);
}

std::optional<GraphicsInitResult> PrimaryInitFull(GraphicsParams params) {
  const auto maybe_params = WindowBackendCreate(params);
  if (!maybe_params) {
    return std::nullopt;
  }
  params = maybe_params.value();

  const auto *driver = SDL_GetRenderDriver(params.api);
  RenderState().primary_renderer =
      SDL_CreateRenderer(WindowBackendSDL(), driver);
  if (RenderState().primary_renderer == nullptr) {
    const auto driver_str = WindowBackendSDLRendererName(params.api);
    const auto label = GraphicsBackendAPILabel(driver_str);
    const auto *api = label.data();
    logging::Critical(kLogCat, "Error creating {} renderer: {}", api,
                      SDL_GetError());
    return PrimaryCleanup();
  }
  const auto driver_str = GraphicsBackendAPIString();
  logging::Info(kLogCat, "Using SDL renderer: {}",
                GraphicsBackendAPILabel(driver_str));

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
    const auto label = GraphicsBackendAPILabel(driver_str);
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
        SafeDestroy(SDL_DestroySurface, RenderState().software_surface);
    RenderState().software_surface =
        SDL_CreateSurface(kGameResolution.w, kGameResolution.h, sdl_format);
    if (RenderState().software_surface == nullptr) {
      logging::SdlError(kLogCat,
                        "Error creating surface for software rendering");
      return PrimaryCleanup();
    }
  }

  const auto res_new = params.ScaledRes();
  const auto scale_geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
  params.SetFlag(
      GraphicsParamFlags::ScaleGeometry,
      static_cast<std::underlying_type_t<GraphicsParamFlags>>(scale_geometry));
  PrimarySetBorderlessFullscreenFit(params, res_new);

  return GraphicsInitResult{.live = params, .reload_surfaces = true};
}

} // namespace

std::optional<GraphicsInitResult>
GraphicsBackendInit(std::optional<const GraphicsParams> maybe_prev,
                    GraphicsParams params) {
  const auto reinit_full = [](const GraphicsParams &params) {
    PrimaryCleanup();
    return PrimaryInitFull(params);
  };

  const auto fs_new = params.FullscreenFlags();

  // This is the only place that applies to both a full init and a partial
  // update later...
  if (fs_new.fullscreen && fs_new.exclusive) {
    SDL_HideCursor();
  } else {
    SDL_ShowCursor();
  }

  if (!maybe_prev) {
    return PrimaryInitFull(params);
  }
  const auto &prev = maybe_prev.value();
  const auto fs_prev = prev.FullscreenFlags();

  // API changes need a complete reinit.
  if (prev.api != params.api) {
    return reinit_full(params);
  }

  // As do a few things when switching to exclusive fullscreen.
  if (fs_new.fullscreen && fs_new.exclusive) {
    // The Direct3D renderer can only launch into exclusive fullscreen on
    // the same display the window was spawned on, so let's just throw it
    // away.
    const auto name = WindowBackendSDLRendererName(params.api);
    if (!fs_prev.fullscreen && (name == "direct3d")) {
      return reinit_full(params);
    }
  }

  // The following parameters can be changed on the fly, but we don't want to
  // reflect modifications of any parameters we don't care about.
  GraphicsInitResult ret = {.live = prev, .reload_surfaces = false};
  using F = GraphicsParamFlags;

  // Apply fullscreen changes first, as exclusive fullscreen affects the
  // display size calculated by ScaledRes().
  if ((fs_prev.fullscreen != fs_new.fullscreen) ||
      (fs_prev.exclusive != fs_new.exclusive)) {
    const auto maybe_fs_actual = HelpSwitchFullscreen(fs_prev, fs_new);
    if (maybe_fs_actual) {
      const auto &fs_actual = maybe_fs_actual.value();

      // If we clipped on the raw renderer, the clipping rectangle won't
      // match the current resolution anymore.
      SDL_SetRenderClipRect(RenderState().primary_renderer, nullptr);

      ret.live.SetFlag(F::Fullscreen,
                       static_cast<std::underlying_type_t<GraphicsParamFlags>>(
                           fs_actual.fullscreen));
      ret.live.SetFlag(F::FullscreenExclusive,
                       static_cast<std::underlying_type_t<GraphicsParamFlags>>(
                           fs_actual.exclusive));
    }
  }

  auto *window = WindowBackendSDL();
  WindowSize res_prev{};
  SDL_GetWindowSize(window, &res_prev.w, &res_prev.h);

  const auto res_new = params.ScaledRes();
  const bool res_changed = (res_prev != res_new);
  if (res_changed && !fs_new.fullscreen) {
    PixelPoint topleft{};
    SDL_GetWindowPosition(window, &topleft.x, &topleft.y);
    SDL_SetWindowSize(window, res_new.w, res_new.h);

    // Necessary for the X11 backend.
    SDL_SyncWindow(window);

    topleft = RepositionAfterScale(topleft, res_prev, res_new);
    ret.live.left = topleft.x;
    ret.live.top = topleft.y;
  } else {
    ret.live.left = params.left;
    ret.live.top = params.top;
  }

  // Should always be applied unconditionally so that the user can change
  // from the maximum scale value to 0, both of which result in the same
  // scaled resolution.
  ret.live.window_scale_4x = params.window_scale_4x;

  const auto scale_geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
  ret.live.SetFlag(
      F::ScaleGeometry,
      static_cast<std::underlying_type_t<GraphicsParamFlags>>(scale_geometry));

  PrimarySetBorderlessFullscreenFit(params, res_new);
  ret.live.SetFlag(F::FullscreenFit, std::to_underlying(fs_new.fit));

  return ret;
}

void GraphicsBackendCleanup() {
  PrimaryCleanup();
  DestroySoftwareRenderer();
  RenderState().software_surface =
      SafeDestroy(SDL_DestroySurface, RenderState().software_surface);
}
/// --------------------------

/// General
/// -------

void GraphicsBackendClear(uint8_t /*unused*/, Rgb col) {
  SDL_SetRenderDrawColor(*RenderState().renderer, col.r, col.g, col.b, 0xFF);
  SDL_RenderClear(*RenderState().renderer);
}

void GraphicsBackendSetClip(const WindowLtrb &rect) {
  if ((*RenderState().renderer) == nullptr) {
    return;
  }
  const auto sdl_rect = HelpRectTo<SDL_Rect>(rect);
  SDL_SetRenderClipRect(*RenderState().renderer, &sdl_rect);
}

std::string_view GraphicsBackendAPIString() {
  // More efficient than the hash table insertion done by
  // SDL_GetRendererName().
  assert(RenderState().primary_renderer);
  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  return SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, nullptr);
}

namespace {

void TakeScreenshot() {
  SDL_FlushRenderer(*RenderState().renderer);

  if (RenderState().software_renderer != nullptr) {
    // Software rendering is the ideal case for screenshots, because we
    // already have a system-memory surface we can save.
    GraphicsScreenshotSave(RenderState().software_surface);
    return;
  }

  SDL_Surface *src =
      SDL_RenderReadPixels(RenderState().primary_renderer, nullptr);
  if (src == nullptr) {
    logging::SdlError(kLogCat, "Error taking screenshot");
    return;
  }
  auto src_guard = util::MakeGuard(src, SDL_DestroySurface);
  GraphicsScreenshotSave(src);
}

} // namespace

void GraphicsBackendFlip(bool take_screenshot) {
  if (take_screenshot) {
    TakeScreenshot();
  }
  if (RenderState().software_renderer != nullptr) {
    SDL_FlushRenderer(RenderState().software_renderer);
    if (SDL_MUSTLOCK(RenderState().software_surface)) {
      SDL_LockSurface(RenderState().software_surface);
    }
    auto unlock_guard = util::MakeGuard([&] {
      if (SDL_MUSTLOCK(RenderState().software_surface)) {
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
    GraphicsBackendClear(0, Rgb{.r = 0, .g = 0, .b = 0});

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
                             const PixelSize &size) {
  auto &tex = RenderState().textures[sid];
  tex = SafeDestroy(SDL_DestroyTexture, tex);

  tex = SDL_CreateTexture(*RenderState().renderer, fmt,
                          SDL_TEXTUREACCESS_STREAMING, size.w, size.h);
  if (tex == nullptr) {
    logging::SdlError(kLogCat, "Error creating blank texture");
    return false;
  }
  TexturePostInit(*tex, *RenderState().renderer);
  if (!SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)) {
    logging::SdlError(kLogCat, "Error enabling alpha blending for texture");
    return false;
  }
  return true;
}

} // namespace

bool GraphicsSurfaceCreateUninitialized(SurfaceId sid, const PixelSize &size) {
  return CreateTextureWithFormat(sid, SDL_PIXELFORMAT_ARGB8888, size);
}

bool GraphicsSurfaceLoad(SurfaceId sid, BmpOwned bmp) {
  auto &tex = RenderState().textures[sid];
  tex = SafeDestroy(SDL_DestroyTexture, tex);

  auto *rwops = SDL_IOFromMem(bmp.buffer.data(), bmp.buffer.size());
  auto *surf = SDL_LoadBMP_IO(rwops, true);
  auto surf_guard = util::MakeGuard(surf, SDL_DestroySurface);

  if (surf->format == SDL_PIXELFORMAT_INDEX8) {
    // The transparent pixel is in the top-left corner.
    if (SDL_MUSTLOCK(surf)) {
      SDL_LockSurface(surf);
    }
    const auto key = static_cast<uint8_t *>(surf->pixels)[0];
    if (SDL_MUSTLOCK(surf)) {
      SDL_UnlockSurface(surf);
    }
    SDL_SetSurfaceColorKey(surf, true, key);
  }

  tex = SDL_CreateTextureFromSurface(*RenderState().renderer, surf);
  if (tex == nullptr) {
    logging::SdlError(kLogCat, "Error loading .BMP as texture");
    return false;
  }
  TexturePostInit(*tex, *RenderState().renderer);
  return true;
}

bool GraphicsSurfaceUpdate(
    SurfaceId sid, const PixelLtwh *subrect,
    std::tuple<const std::byte *, size_t> pixels) noexcept {
  const auto [buf, pitch] = pixels;
  if (pitch > std::numeric_limits<int>::max()) {
    logging::Critical(kLogCat, "Pitch of {} bytes does not fit into an integer",
                      pitch);
    return false;
  }

  auto *tex = RenderState().textures[sid];
  if (subrect == nullptr) {
    return (static_cast<int>(SDL_UpdateTexture(tex, nullptr, buf, pitch)) == 0);
  }
  const auto rect = HelpRectTo<SDL_Rect>(*subrect);
  return (static_cast<int>(SDL_UpdateTexture(tex, &rect, buf, pitch)) == 0);
}

PixelSize GraphicsSurfaceSize(SurfaceId sid) {
  auto *tex = RenderState().textures[sid];
  if (tex == nullptr) {
    return {.w = 0, .h = 0};
  }
  float w = 0;
  float h = 0;
  if (!SDL_GetTextureSize(tex, &w, &h)) {
    return {.w = 0, .h = 0};
  }
  return {
      .w = static_cast<PixelCoord>(w),
      .h = static_cast<PixelCoord>(h),
  };
}

bool GraphicsSurfaceBlit(WindowPoint topleft, SurfaceId sid,
                         const PixelLtrb &src) {
  auto *const tex = RenderState().textures[sid];
  const auto rect_src = HelpRectTo<SDL_FRect>(src);
  const SDL_FRect rect_dst = {
      .x = static_cast<float>(topleft.x),
      .y = static_cast<float>(topleft.y),
      .w = static_cast<float>(rect_src.w),
      .h = static_cast<float>(rect_src.h),
  };
  return SDL_RenderTexture(*RenderState().renderer, tex, &rect_src, &rect_dst);
}

void GraphicsSurfaceBlitOpaque(WindowPoint topleft, SurfaceId sid,
                               const PixelLtrb &src) {
  auto *tex = RenderState().textures[sid];
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

// NOLINTBEGIN(misc-include-cleaner) - GDI symbols are provided by windows.h
// through the platform surface header.
#ifdef WIN32
// Win32 GDI text rendering bridge
// -------------------------------

#include "platform/windows/surface_gdi.h"

// SDL textures only support transparency via alpha blending, and the only
// alpha-blended formats available on any SDL_Renderer backend in a Windows
// build of SDL 2.30.6 are 32-bit ones. GDI also exclusively uses the BGRX
// memory order for 32-bit bitmaps. Might as well limit the GDI code to that
// one specific format then.
constexpr auto kGdiTextBpp = 32;
constexpr auto kGdiTextSdlFormat = SDL_PIXELFORMAT_ARGB8888;

namespace {

struct GdiTextState {
  SurfaceGdi surface;
  uint32_t color_key = 0;
  uint32_t alpha_mask = 0;
};

GdiTextState &GdiText() {
  static GdiTextState state;
  return state;
}

} // namespace

SurfaceGdi &GraphicsSurfaceGdiTextSurface() noexcept {
  return GdiText().surface;
}

bool GraphicsSurfaceGdiTextCreate(int32_t w, int32_t h, Rgb colorkey) {
  auto &state = GdiText();
  auto &surface = state.surface;
  surface.Delete();

  if (!std::ranges::contains(RenderState().primary_formats,
                             kGdiTextSdlFormat)) {
    logging::Critical(
        kLogCat,
        "Renderer \"{}\" does not support the BGRA8888 pixel format "
        "required for rendering text via GDI",
        GraphicsBackendAPIString());
    return false;
  };

  const auto *format_struct = SDL_GetPixelFormatDetails(kGdiTextSdlFormat);
  if (format_struct == nullptr) {
    logging::SdlError(kLogCat,
                      "Error retrieving format structure for GDI text surface");
    return false;
  }

  // GDI always sets the "alpha channel" to 0. Removing it from the color key
  // as well saves an OR operation in the alpha fixing loop.
  state.alpha_mask = format_struct->Amask;
  state.color_key =
      (SDL_MapRGB(format_struct, nullptr, colorkey.r, colorkey.g, colorkey.b) &
       ~state.alpha_mask);

  const BITMAPINFOHEADER bmi = {
      .biSize = sizeof(bmi),
      .biWidth = w,
      .biHeight = -h,
      .biPlanes = 1,
      .biBitCount = kGdiTextBpp,
      .biCompression = BI_RGB,
  };
  const auto *bi = reinterpret_cast<const BITMAPINFO *>(&bmi);
  void *dib_bits = nullptr;
  surface.img = CreateDIBSection(surface.dc, bi, 0, &dib_bits, nullptr, 0);
  if (surface.img == nullptr) {
    logging::Critical(kLogCat, "Error creating GDI text surface");
    return false;
  }
  surface.size = {.w = w, .h = h};
  surface.stock_img = SelectObject(surface.dc, surface.img);
  return CreateTextureWithFormat(SurfaceId::Text, kGdiTextSdlFormat,
                                 {.w = w, .h = h});
}

bool GraphicsSurfaceGdiTextUpdate(const PixelLtwh &r) noexcept {
  auto &state = GdiText();
  DIBSECTION dib;
  if (!GetObject(state.surface.img, sizeof(DIBSECTION), &dib)) {
    return false;
  }

  auto *pixels =
      (static_cast<std::byte *>(dib.dsBm.bmBits) +
       (static_cast<std::ptrdiff_t>(r.top) * dib.dsBm.bmWidthBytes) +
       (static_cast<std::ptrdiff_t>(r.left) * (dib.dsBmih.biBitCount / 8)));

  static_assert((kGdiTextBpp == 32), "Only tested for 32-bit.");
  const auto w = static_cast<size_t>(r.w);
  const auto h = static_cast<size_t>(r.h);
  auto *row_p = pixels;
  for (const auto y : std::views::iota(0U, h)) {
    auto pixels_in_row = std::span(reinterpret_cast<uint32_t *>(row_p), w);
    for (auto &pixel : pixels_in_row) {
      if (pixel != state.color_key) {
        pixel |= state.alpha_mask;
      }
    }
    row_p += dib.dsBm.bmWidthBytes;
  };
  const auto pitch = static_cast<size_t>(dib.dsBm.bmWidthBytes);
  return GraphicsSurfaceUpdate(SurfaceId::Text, &r, {pixels, pitch});
}
// -------------------------------
#endif
// NOLINTEND(misc-include-cleaner)
/// --------

/// Geometry
/// --------

namespace {

void DrawGeometry(TrianglePrimitive tp, VertexXySpan<> xys,
                  VertexRgbaSpan<> colors) {
#pragma warning(suppress : 26494) // type.5
  std::array<SDL_FPoint, kMaxTriangles> sdl_vertices{};

  const auto vertex_count = xys.size();
  const auto sdl_colors = HelpColorsFrom(colors);
  const auto indices = kTriangleIndices[tp];
  const auto index_count = TriangleIndexCount(vertex_count);
  assert(vertex_count <= std::size(sdl_vertices));
  assert(index_count <= indices.size());
  assert((colors.size() == 1) || (colors.size() == vertex_count));

  // Work around SDL's weird -0.5f offset...
  float offset_x = NAN;
  float offset_y = NAN;
  SDL_GetRenderScale(*RenderState().renderer, &offset_x, &offset_y);
  offset_x = (1.0F / (2.0F * offset_x));
  offset_y = (1.0F / (2.0F * offset_y));
  auto *sdl = sdl_vertices.data();
  for (const auto &game : xys) {
    *(sdl++) = {.x = (game.x + offset_x), .y = (game.y + offset_y)};
  }

  SDL_RenderGeometryRaw(
      *RenderState().renderer, nullptr, &sdl_vertices[0].x, sizeof(SDL_FPoint),
      sdl_colors.data(), ((sdl_colors.size() == 1) ? 0 : sizeof(SdlColor)),
      nullptr, 0, vertex_count, indices.data(), index_count, sizeof(IndexType));
}

void DrawWithAlpha(auto func) {
  SDL_SetRenderDrawBlendMode(*RenderState().renderer, RenderState().alpha_mode);
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b,
                         RenderState().color.a);
  func();
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
  SDL_SetRenderDrawBlendMode(*RenderState().renderer, SDL_BLENDMODE_NONE);
}

} // namespace

void geometry::SetColor(Rgb216 col) {
  const auto rgb = col.ToRgb();
  RenderState().color.r = rgb.r;
  RenderState().color.g = rgb.g;
  RenderState().color.b = rgb.b;
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
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
  SDL_RenderLine(*RenderState().renderer, x1, y1, x2, y2);
}

void geometry::DrawBox(int x1, int y1, int x2, int y2) {
  const SDL_FRect rect = {
      .x = static_cast<float>(x1),
      .y = static_cast<float>(y1),
      .w = static_cast<float>(x2 - x1),
      .h = static_cast<float>(y2 - y1),
  };
  SDL_RenderFillRect(*RenderState().renderer, &rect);
}

void geometry::DrawBoxA(int x1, int y1, int x2, int y2) {
  DrawWithAlpha([&] { DrawBox(x1, y1, x2, y2); });
}

void geometry::DrawLineStrip(VertexXySpan<> xys) {
  const auto points = HelpFPointsFrom(xys);
  SDL_RenderLines(*RenderState().renderer, points.data(), points.size());
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
      VertexXy{static_cast<VertexCoord>(x + 0), static_cast<VertexCoord>(y1)},
      VertexXy{static_cast<VertexCoord>(x + 0), static_cast<VertexCoord>(y2)},
      VertexXy{static_cast<VertexCoord>(x + 1), static_cast<VertexCoord>(y1)},
      VertexXy{static_cast<VertexCoord>(x + 1), static_cast<VertexCoord>(y2)},
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

bool GraphicsBackendPixelAccessStart() {
  if (RenderState().software_renderer != nullptr) {
    return true;
  }
  RenderState().software_renderer =
      SDL_CreateSoftwareRenderer(RenderState().software_surface);
  if (RenderState().software_renderer == nullptr) {
    logging::SdlError(kLogCat, "Error creating software renderer");
    return DestroySoftwareRenderer();
  }
  SwitchActiveRenderer(&RenderState().software_renderer);
  return (EnsureSoftwareTexture() != nullptr);
}

bool GraphicsBackendPixelAccessEnd() {
  if (RenderState().software_renderer == nullptr) {
    return true;
  }
  SwitchActiveRenderer(&RenderState().primary_renderer);
  DestroySoftwareRenderer();
  return true;
}

std::tuple<std::byte *, size_t> GraphicsBackendPixelAccessLock() {
  // Necessary in SDL 3!
  SDL_FlushRenderer(RenderState().software_renderer);

  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    if (!SDL_LockSurface(RenderState().software_surface)) {
      logging::SdlError(kLogCat, "Error locking CPU backbuffer");
      return {nullptr, 0};
    }
  }
  auto *pixels =
      static_cast<std::byte *>(RenderState().software_surface->pixels);
  return {pixels, RenderState().software_surface->pitch};
}

void GraphicsBackendPixelAccessUnlock() {
  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    SDL_UnlockSurface(RenderState().software_surface);
  }
}
/// ------------------------------------
