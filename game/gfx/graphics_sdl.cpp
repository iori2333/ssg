///
/// Graphics via SDL_Renderer
///

#include <format>
#include <string>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_render.h>

using SDL_COLOR = SDL_FColor;

#include "format_bmp.h"
#include "graphics_sdl.h"
#include "window_backend.h"
#include "window_sdl.h"

#include "gfx/constants.h"
#include "platform/text_backend.h"
#include "sys/log.h"
#include "util/enum_array.h"
#include "util/guard.h"

static constexpr auto LOG_CAT = logging::Channel::Graphics;

namespace {

struct RendererState {
  SDL_ScaleMode texture_scale_mode = SDL_SCALEMODE_NEAREST;
  SDL_Renderer *primary_renderer = nullptr;
  std::span<const SDL_PixelFormat> primary_formats;
  SDL_Texture *primary_texture = nullptr;
  SDL_Renderer *software_renderer = nullptr;
  SDL_Surface *software_surface = nullptr;
  SDL_Texture *software_texture = nullptr;
  SDL_Renderer **renderer = &primary_renderer;
  util::EnumArray<SDL_Texture *, SURFACE_ID> textures;
  GraphicsGeometry geometry;
  RGBA color = {0, 0, 0, 0xFF};
  SDL_BlendMode alpha_mode = SDL_BLENDMODE_NONE;
};

RendererState &RenderState() {
  static RendererState state;
  return state;
}

} // namespace

GraphicsGeometry &Geometry() { return RenderState().geometry; }

static SDL_Texture *EnsureSoftwareTexture(void);

// Compile-time index buffers
// --------------------------

using INDEX_TYPE = uint8_t;

constexpr int TriangleIndexCount(size_t vertex_count) {
  return ((vertex_count - 3 + 1) * 3);
}

constinit const auto TRIANGLE_FAN = ([] {
  constexpr INDEX_TYPE max = GRP_TRIANGLES_MAX;
  std::array<INDEX_TYPE, TriangleIndexCount(max)> ret;
  auto ret_p = ret.begin();
  for (const auto &i : std::views::iota(0u, (max - 2u))) {
    *(ret_p++) = 0;
    *(ret_p++) = (i + 1);
    *(ret_p++) = (i + 2);
  }
  return ret;
})();

constinit const auto TRIANGLE_STRIP = ([] {
  constexpr INDEX_TYPE max = GRP_TRIANGLES_MAX;
  std::array<INDEX_TYPE, TriangleIndexCount(max)> ret;
  auto ret_p = ret.begin();
  for (const auto &i : std::views::iota(0u, (max - 2u))) {
    *(ret_p++) = (i + 0);
    *(ret_p++) = (i + 1);
    *(ret_p++) = (i + 2);
  }
  return ret;
})();

constinit const util::EnumArray<std::span<const INDEX_TYPE>, TRIANGLE_PRIMITIVE>
    INDICES = {TRIANGLE_FAN, TRIANGLE_STRIP};
// --------------------------

// Helpers
// -------

template <typename Rect> Rect HelpRectTo(const PIXEL_LTWH &o) noexcept {
  return Rect{
      .x = static_cast<decltype(Rect::x)>(o.left),
      .y = static_cast<decltype(Rect::y)>(o.top),
      .w = static_cast<decltype(Rect::w)>(o.w),
      .h = static_cast<decltype(Rect::h)>(o.h),
  };
}

template <typename Rect> Rect HelpRectTo(const PIXEL_LTRB &o) {
  return Rect{
      .x = static_cast<decltype(Rect::x)>(o.left),
      .y = static_cast<decltype(Rect::y)>(o.top),
      .w = static_cast<decltype(Rect::w)>(o.right - o.left),
      .h = static_cast<decltype(Rect::h)>(o.bottom - o.top),
  };
}

std::span<const SDL_FPoint> HelpFPointsFrom(VERTEX_XY_SPAN<> sp) {
  using GT = decltype(sp)::value_type;
  static_assert(sizeof(SDL_FPoint) == sizeof(GT));
  static_assert(std::is_same_v<decltype(SDL_FPoint::x), decltype(GT::x)>);
  static_assert(std::is_same_v<decltype(SDL_FPoint::y), decltype(GT::y)>);
  return {reinterpret_cast<const SDL_FPoint *>(sp.data()), sp.size()};
}

std::span<const SDL_COLOR> HelpColorsFrom(VERTEX_RGBA_SPAN<> sp) {
  using GT = decltype(sp)::value_type;
  static_assert(sizeof(SDL_COLOR) == sizeof(GT));
  static_assert(std::is_same_v<decltype(SDL_COLOR::r), decltype(GT::r)>);
  static_assert(std::is_same_v<decltype(SDL_COLOR::g), decltype(GT::g)>);
  static_assert(std::is_same_v<decltype(SDL_COLOR::b), decltype(GT::b)>);
  static_assert(std::is_same_v<decltype(SDL_COLOR::a), decltype(GT::a)>);
  return {std::bit_cast<const SDL_COLOR *>(sp.data()), sp.size()};
}

SDL_Texture *TexturePostInit(SDL_Texture &tex, SDL_Renderer *renderer) {
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
  } else if ((renderer == RenderState().primary_renderer) &&
             RenderState().primary_texture) {
    return SDL_SetRenderTarget(RenderState().primary_renderer,
                               RenderState().primary_texture);
  }
  return true;
}

void SwitchActiveRenderer(SDL_Renderer **new_renderer) {
  for (auto &tex : RenderState().textures) {
    if (!tex) {
      continue;
    }
    const auto *renderer = SDL_GetRendererFromTexture(tex);
    if (tex && (renderer == *RenderState().renderer)) {
      tex = SafeDestroy(SDL_DestroyTexture, tex);
    }
  }
  SetRenderTargetFor(*new_renderer);
  RenderState().renderer = new_renderer;
}

std::optional<GRAPHICS_FULLSCREEN_FLAGS>
HelpSwitchFullscreen(const GRAPHICS_FULLSCREEN_FLAGS &fs_prev,
                     const GRAPHICS_FULLSCREEN_FLAGS &fs_new) {
  auto *window = WndBackend_SDL();

  if (!fs_prev.fullscreen && fs_new.fullscreen) {
    WndBackend_RememberTopleft(HelpGetWindowPosition(window));
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

constexpr std::pair<std::string_view, std::string_view> API_NICE[] = {
    {"direct3d", "Direct3D 9"},    {"direct3d11", "Direct3D 11"},
    {"direct3d12", "Direct3D 12"}, {"software", "Software"},
    {"vulkan", "Vulkan"},
};

namespace APIVersions {
struct VERSION {
  std::string_view name_sdl;
  const char *name_pretty;
  void (*update)(VERSION &self);
  std::string buf;
};

void UpdateGPU(VERSION &self) {
  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  auto *gpu_device = static_cast<SDL_GPUDevice *>(SDL_GetPointerProperty(
      props, SDL_PROP_RENDERER_GPU_DEVICE_POINTER, nullptr));

  std::string_view device_name = SDL_GetGPUDeviceDriver(gpu_device);
  for (const auto &nice : API_NICE) {
    if (nice.first == device_name) {
      device_name = nice.second;
      break;
    }
  }
  const auto *via_name = (device_name.empty() ? "?" : device_name.data());
  self.buf = std::format("GPU ({})", via_name);
}

void UpdateOpenGL(VERSION &self) {
  int major = 0;
  int minor = 0;
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
  SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);

  self.buf = std::format("{} {}.{}", self.name_pretty, major, minor);
}

#define TARGET_(pretty, maj, min) pretty " ~" #maj "." #min
#define TARGET(pretty, maj, min) TARGET_(pretty, maj, min)

VERSION Versions[] = {
    {"gpu", nullptr, UpdateGPU, "GPU"},
    {"opengl", "OpenGL", UpdateOpenGL,
     TARGET("OpenGL", OPENGL_TARGET_CORE_MAJ, OPENGL_TARGET_CORE_MIN)},
    {"opengles2", "OpenGL ES", UpdateOpenGL,
     TARGET("OpenGL ES", 2, OPENGL_TARGET_ES2_MIN)},
};

#undef TARGET
#undef TARGET_

void Update(std::string_view driver_str) {
  auto *version = std::ranges::find(Versions, driver_str, &VERSION::name_sdl);
  if (version == std::end(Versions)) {
    return;
  }
  version->update(*version);
}
} // namespace APIVersions
// --------------------------

/// Enumeration and pre-initialization queries
/// ------------------------------------------

bool GrpBackend_Enum(void) {
  // Any SDL-specific initialization was already done as part of
  // SDL_Init(SDL_INIT_VIDEO).
  return true;
}

int8_t GrpBackend_APICount(void) { return SDL_GetNumRenderDrivers(); }

std::string_view GrpBackend_APILabel(std::string_view api) {
  for (const auto &nice : API_NICE) {
    if (nice.first == api) {
      return nice.second;
    }
  }
  for (const auto &nice : APIVersions::Versions) {
    if (nice.name_sdl == api) {
      return nice.buf;
    }
  }
  return api;
}

int8_t GrpBackend_APIID(std::string_view api) {
  for (const auto i : std::views::iota(0, SDL_GetNumRenderDrivers())) {
    if (GrpBackend_APIString(i) == api) {
      return i;
    }
  }
  return -1;
}

std::string_view GrpBackend_APIString(int8_t id) {
  const auto *ret = SDL_GetRenderDriver(id);
  return (ret ? ret : std::string_view{});
}

PIXEL_SIZE GrpBackend_DisplaySize(bool fullscreen) {
  SDL_Rect rect{};
  const auto display_i = HelpGetDisplayForWindow();
  if (fullscreen) {
    const auto *display_mode = SDL_GetDesktopDisplayMode(display_i);
    if (!display_mode) {
      logging::SdlError(LOG_CAT, "Error retrieving display size");
      return GRP_RES;
    }
    return {.w = display_mode->w, .h = display_mode->h};
  }

  if (!SDL_GetDisplayUsableBounds(display_i, &rect)) {
    logging::SdlError(LOG_CAT, "Error retrieving display size");
    return GRP_RES;
  }
  return {.w = rect.w, .h = rect.h};
}
/// ------------------------------------------

/// Initialization and cleanup
/// --------------------------

bool DestroySoftwareRenderer(void) {
  RenderState().software_renderer =
      SafeDestroy(SDL_DestroyRenderer, RenderState().software_renderer);
  RenderState().software_texture =
      SafeDestroy(SDL_DestroyTexture, RenderState().software_texture);
  return false;
}

std::nullopt_t PrimaryCleanup(void) {
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
  WndBackend_Cleanup();

  return std::nullopt;
}

// Returns the new `SCALE_GEOMETRY` flag.
bool PrimarySetScale(bool geometry, const WINDOW_SIZE &scaled_res) {
  const auto set_geometry = [] {
    RenderState().primary_texture =
        SafeDestroy(SDL_DestroyTexture, RenderState().primary_texture);
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer, GRP_RES.w,
                                     GRP_RES.h,
                                     SDL_LOGICAL_PRESENTATION_STRETCH);
    return true;
  };

  // Update texture filters
  // ----------------------
  if ((scaled_res.w % GRP_RES.w) || (scaled_res.h % GRP_RES.h)) {
    RenderState().texture_scale_mode = SDL_SCALEMODE_LINEAR;
  } else {
    RenderState().texture_scale_mode = SDL_SCALEMODE_NEAREST;
  }
  for (auto &tex : RenderState().textures) {
    if (tex) {
      SDL_SetTextureScaleMode(tex, RenderState().texture_scale_mode);
    }
  }
  if (RenderState().software_texture) {
    SDL_SetTextureScaleMode(RenderState().software_texture,
                            RenderState().texture_scale_mode);
  }
  if (RenderState().primary_texture) {
    SDL_SetTextureScaleMode(RenderState().primary_texture,
                            RenderState().texture_scale_mode);
  }
  // ----------------------

  if (geometry || (scaled_res == GRP_RES)) {
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

  if (!RenderState().primary_texture) {
    assert(RenderState().software_surface);
    const auto format = RenderState().software_surface->format;
    const auto &res = GRP_RES;
    RenderState().primary_texture =
        SDL_CreateTexture(RenderState().primary_renderer, format,
                          SDL_TEXTUREACCESS_TARGET, res.w, res.h);
    if (!RenderState().primary_texture) {
      logging::SdlError(LOG_CAT, "Error creating native resolution texture");
      return set_geometry();
    }
    SDL_SetTextureScaleMode(RenderState().primary_texture,
                            RenderState().texture_scale_mode);
  }

  // We might be software-rendering.
  if (!SetRenderTargetFor(*RenderState().renderer)) {
    logging::SdlError(LOG_CAT, "Error setting texture as render target");
    return set_geometry();
  }
  return geometry;
}

// Re-centers the window to remain fully on-screen after changing the
// windowed-mode scale factor
PIXEL_POINT RepositionAfterScale(const PIXEL_POINT &topleft_prev,
                                 const WINDOW_SIZE &res_prev,
                                 const WINDOW_SIZE &res_new) {
  auto *window = WndBackend_SDL();
  PIXEL_COORD border_left{};
  PIXEL_COORD border_top{};
  PIXEL_COORD border_right{};
  PIXEL_COORD border_bottom{};
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

void PrimarySetBorderlessFullscreenFit(GRAPHICS_PARAMS params,
                                       const WINDOW_SIZE &scaled_res) {
  using FIT = GRAPHICS_FULLSCREEN_FIT;
  const auto fs = params.FullscreenFlags();

  auto *target = SDL_GetRenderTarget(RenderState().primary_renderer);
  SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);

  if (fs.fullscreen && !fs.exclusive) {
    constexpr auto MODES = [] {
      util::EnumArray<SDL_RendererLogicalPresentation, FIT> ret;
      ret[FIT::INTEGER] = SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
      ret[FIT::ASPECT] = SDL_LOGICAL_PRESENTATION_LETTERBOX;
      ret[FIT::STRETCH] = SDL_LOGICAL_PRESENTATION_STRETCH;
      return ret;
    }();
    SDL_SetRenderLogicalPresentation(RenderState().primary_renderer, GRP_RES.w,
                                     GRP_RES.h, MODES[fs.fit]);
  }
  SDL_SetRenderTarget(RenderState().primary_renderer, target);
}

std::optional<GRAPHICS_INIT_RESULT> PrimaryInitFull(GRAPHICS_PARAMS params) {
  const auto maybe_params = WndBackend_Create(params);
  if (!maybe_params) {
    return std::nullopt;
  }
  params = maybe_params.value();

  const auto *driver = SDL_GetRenderDriver(params.api);
  RenderState().primary_renderer = SDL_CreateRenderer(WndBackend_SDL(), driver);
  if (!RenderState().primary_renderer) {
    const auto driver_str = WndBackend_SDLRendererName(params.api);
    const auto label = GrpBackend_APILabel(driver_str);
    const auto *api = label.data();
    logging::Critical(LOG_CAT, "Error creating {} renderer: {}", api,
                      SDL_GetError());
    return PrimaryCleanup();
  }
  const auto driver_str = GrpBackend_APIString();
  logging::Info(LOG_CAT, "Using SDL renderer: {}",
                GrpBackend_APILabel(driver_str));

  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  const auto *formats_start =
      static_cast<const SDL_PixelFormat *>(SDL_GetPointerProperty(
          props, SDL_PROP_RENDERER_TEXTURE_FORMATS_POINTER, nullptr));
  auto *formats_end = formats_start;
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
    const auto label = GrpBackend_APILabel(driver_str);
    logging::Critical(
        LOG_CAT,
        "The \"{}\" renderer does not support the BGRA8888 pixel format "
        "required for software rendering",
        label);
    return PrimaryCleanup();
  }

  SetRenderTargetFor(RenderState().primary_renderer);
  APIVersions::Update(driver_str);

  // Ensure that the software surface uses the preferred format
  if (!RenderState().software_surface ||
      (RenderState().software_surface->format != sdl_format)) {
    RenderState().software_surface =
        SafeDestroy(SDL_DestroySurface, RenderState().software_surface);
    RenderState().software_surface =
        SDL_CreateSurface(GRP_RES.w, GRP_RES.h, sdl_format);
    if (!RenderState().software_surface) {
      logging::SdlError(LOG_CAT,
                        "Error creating surface for software rendering");
      return PrimaryCleanup();
    }
  }

  const auto res_new = params.ScaledRes();
  const auto geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
  params.SetFlag(GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY, geometry);
  PrimarySetBorderlessFullscreenFit(params, res_new);

  return GRAPHICS_INIT_RESULT{.live = params, .reload_surfaces = true};
}

std::optional<GRAPHICS_INIT_RESULT>
GrpBackend_Init(std::optional<const GRAPHICS_PARAMS> maybe_prev,
                GRAPHICS_PARAMS params) {
  const auto reinit_full = [](const GRAPHICS_PARAMS &params) {
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
    const auto name = WndBackend_SDLRendererName(params.api);
    if (!fs_prev.fullscreen && (name == "direct3d")) {
      return reinit_full(params);
    }
  }

  // The following parameters can be changed on the fly, but we don't want to
  // reflect modifications of any parameters we don't care about.
  GRAPHICS_INIT_RESULT ret = {.live = prev, .reload_surfaces = false};
  using F = GRAPHICS_PARAM_FLAGS;

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

      ret.live.SetFlag(F::FULLSCREEN, fs_actual.fullscreen);
      ret.live.SetFlag(F::FULLSCREEN_EXCLUSIVE, fs_actual.exclusive);
    }
  }

  auto *window = WndBackend_SDL();
  WINDOW_SIZE res_prev{};
  SDL_GetWindowSize(window, &res_prev.w, &res_prev.h);

  const auto res_new = params.ScaledRes();
  const bool res_changed = (res_prev != res_new);
  if (res_changed && !fs_new.fullscreen) {
    PIXEL_POINT topleft{};
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

  const auto geometry = PrimarySetScale(params.ScaleGeometry(), res_new);
  ret.live.SetFlag(F::SCALE_GEOMETRY, geometry);

  PrimarySetBorderlessFullscreenFit(params, res_new);
  ret.live.SetFlag(F::FULLSCREEN_FIT, std::to_underlying(fs_new.fit));

  return ret;
}

void GrpBackend_Cleanup(void) {
  PrimaryCleanup();
  DestroySoftwareRenderer();
  RenderState().software_surface =
      SafeDestroy(SDL_DestroySurface, RenderState().software_surface);
}
/// --------------------------

/// General
/// -------

void GrpBackend_Clear(uint8_t, RGB col) {
  SDL_SetRenderDrawColor(*RenderState().renderer, col.r, col.g, col.b, 0xFF);
  SDL_RenderClear(*RenderState().renderer);
}

void GrpBackend_SetClip(const WINDOW_LTRB &rect) {
  if (!*RenderState().renderer) {
    return;
  }
  const auto sdl_rect = HelpRectTo<SDL_Rect>(rect);
  SDL_SetRenderClipRect(*RenderState().renderer, &sdl_rect);
}

std::string_view GrpBackend_APIString(void) {
  // More efficient than the hash table insertion done by
  // SDL_GetRendererName().
  assert(RenderState().primary_renderer);
  const auto props = SDL_GetRendererProperties(RenderState().primary_renderer);
  return SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, nullptr);
}

void TakeScreenshot(void) {
  SDL_FlushRenderer(*RenderState().renderer);

  if (RenderState().software_renderer) {
    // Software rendering is the ideal case for screenshots, because we
    // already have a system-memory surface we can save.
    Grp_ScreenshotSave(RenderState().software_surface);
    return;
  }

  SDL_Surface *src =
      SDL_RenderReadPixels(RenderState().primary_renderer, nullptr);
  if (!src) {
    logging::SdlError(LOG_CAT, "Error taking screenshot");
    return;
  }
  auto src_guard = util::MakeGuard(src, SDL_DestroySurface);
  Grp_ScreenshotSave(src);
}

void GrpBackend_Flip(bool take_screenshot) {
  if (take_screenshot) {
    TakeScreenshot();
  }
  if (RenderState().software_renderer) {
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
    if (!tex) {
      return;
    }
    SDL_UpdateTexture(tex, nullptr, RenderState().software_surface->pixels,
                      RenderState().software_surface->pitch);
    SDL_RenderTexture(RenderState().primary_renderer,
                      RenderState().software_texture, nullptr, nullptr);
    SDL_RenderPresent(RenderState().primary_renderer);
  } else if (RenderState().primary_texture) {
    SDL_SetRenderTarget(RenderState().primary_renderer, nullptr);

    // In borderless fullscreen mode, the scaled texture may not cover the
    // entire window. Technically, we only need to do this once for every
    // backbuffer after switching the fullscreen fit, but:
    // 1) SDL has no way of querying the length of the swapchain, and
    // 2) you are supposed to do this on every frame anyway, as a lot of
    //    GPUs can use clearing as a performance hint.
    // Let's measure the performance impact on windowed mode some other
    // time...
    GrpBackend_Clear(0, RGB{0, 0, 0});

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

bool CreateTextureWithFormat(SURFACE_ID sid, SDL_PixelFormat fmt,
                             const PIXEL_SIZE &size) {
  auto &tex = RenderState().textures[sid];
  tex = SafeDestroy(SDL_DestroyTexture, tex);

  tex = SDL_CreateTexture(*RenderState().renderer, fmt,
                          SDL_TEXTUREACCESS_STREAMING, size.w, size.h);
  if (!tex) {
    logging::SdlError(LOG_CAT, "Error creating blank texture");
    return false;
  }
  TexturePostInit(*tex, *RenderState().renderer);
  if (!SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND)) {
    logging::SdlError(LOG_CAT, "Error enabling alpha blending for texture");
    return false;
  }
  return true;
}

bool GrpSurface_CreateUninitialized(SURFACE_ID sid, const PIXEL_SIZE &size) {
  return CreateTextureWithFormat(sid, SDL_PIXELFORMAT_ARGB8888, size);
}

bool GrpSurface_Load(SURFACE_ID sid, BMP_OWNED &&bmp) {
  auto &tex = RenderState().textures[sid];
  tex = SafeDestroy(SDL_DestroyTexture, tex);

  auto *rwops = SDL_IOFromMem(bmp.buffer.data(), bmp.buffer.size());
  auto *surf = SDL_LoadBMP_IO(rwops, 1);
  auto surf_guard = util::MakeGuard(surf, SDL_DestroySurface);
  std::ignore = std::move(bmp);

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
  if (!tex) {
    logging::SdlError(LOG_CAT, "Error loading .BMP as texture");
    return false;
  }
  TexturePostInit(*tex, *RenderState().renderer);
  return true;
}

bool GrpSurface_Update(SURFACE_ID sid, const PIXEL_LTWH *subrect,
                       std::tuple<const std::byte *, size_t> pixels) noexcept {
  const auto [buf, pitch] = pixels;
  if (pitch > std::numeric_limits<int>::max()) {
    logging::Critical(LOG_CAT, "Pitch of {} bytes does not fit into an integer",
                      pitch);
    return false;
  }

  auto *tex = RenderState().textures[sid];
  if (!subrect) {
    return (SDL_UpdateTexture(tex, nullptr, buf, pitch) == 0);
  }
  const auto rect = HelpRectTo<SDL_Rect>(*subrect);
  return (SDL_UpdateTexture(tex, &rect, buf, pitch) == 0);
}

PIXEL_SIZE GrpSurface_Size(SURFACE_ID sid) {
  auto *tex = RenderState().textures[sid];
  if (!tex) {
    return {0, 0};
  }
  float w = 0;
  float h = 0;
  if (!SDL_GetTextureSize(tex, &w, &h)) {
    return {0, 0};
  }
  return {
      .w = static_cast<PIXEL_COORD>(w),
      .h = static_cast<PIXEL_COORD>(h),
  };
}

bool GrpSurface_Blit(WINDOW_POINT topleft, SURFACE_ID sid,
                     const PIXEL_LTRB &src) {
  const auto tex = RenderState().textures[sid];
  const auto rect_src = HelpRectTo<SDL_FRect>(src);
  const SDL_FRect rect_dst = {
      .x = static_cast<float>(topleft.x),
      .y = static_cast<float>(topleft.y),
      .w = static_cast<float>(rect_src.w),
      .h = static_cast<float>(rect_src.h),
  };
  return SDL_RenderTexture(*RenderState().renderer, tex, &rect_src, &rect_dst);
}

void GrpSurface_BlitOpaque(WINDOW_POINT topleft, SURFACE_ID sid,
                           const PIXEL_LTRB &src) {
  auto *tex = RenderState().textures[sid];
  SDL_BlendMode prev{};
  SDL_GetTextureBlendMode(tex, &prev);
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
  GrpSurface_Blit(topleft, sid, src);
  SDL_SetTextureBlendMode(tex, prev);
}

void GrpSurface_SetColorMod(SURFACE_ID sid, uint8_t r, uint8_t g, uint8_t b) {
  SDL_SetTextureColorMod(RenderState().textures[sid], r, g, b);
}

#ifdef WIN32
// Win32 GDI text rendering bridge
// -------------------------------

#include "platform/windows/surface_gdi.h"

// SDL textures only support transparency via alpha blending, and the only
// alpha-blended formats available on any SDL_Renderer backend in a Windows
// build of SDL 2.30.6 are 32-bit ones. GDI also exclusively uses the BGRX
// memory order for 32-bit bitmaps. Might as well limit the GDI code to that
// one specific format then.
static constexpr auto GDITEXT_BPP = 32;
static constexpr auto GDITEXT_SDL_FORMAT = SDL_PIXELFORMAT_ARGB8888;

struct GdiTextState {
  SURFACE_GDI surface;
  uint32_t color_key = 0;
  uint32_t alpha_mask = 0;
};

GdiTextState &GdiText() {
  static GdiTextState state;
  return state;
}

SURFACE_GDI &GrpSurface_GDIText_Surface(void) noexcept {
  return GdiText().surface;
}

bool GrpSurface_GDIText_Create(int32_t w, int32_t h, RGB colorkey) {
  auto &state = GdiText();
  auto &surface = state.surface;
  surface.Delete();

  if (!std::ranges::contains(RenderState().primary_formats,
                             GDITEXT_SDL_FORMAT)) {
    logging::Critical(
        LOG_CAT,
        "Renderer \"{}\" does not support the BGRA8888 pixel format "
        "required for rendering text via GDI",
        GrpBackend_APIString());
    return false;
  };

  const auto *format_struct = SDL_GetPixelFormatDetails(GDITEXT_SDL_FORMAT);
  if (!format_struct) {
    logging::SdlError(LOG_CAT,
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
      .biBitCount = GDITEXT_BPP,
      .biCompression = BI_RGB,
  };
  const auto *bi = reinterpret_cast<const BITMAPINFO *>(&bmi);
  void *dib_bits = nullptr;
  surface.img = CreateDIBSection(surface.dc, bi, 0, &dib_bits, nullptr, 0);
  if (!surface.img) {
    logging::Critical(LOG_CAT, "Error creating GDI text surface");
    return false;
  }
  surface.size = {w, h};
  surface.stock_img = SelectObject(surface.dc, surface.img);
  return CreateTextureWithFormat(SURFACE_ID::TEXT, GDITEXT_SDL_FORMAT, {w, h});
}

bool GrpSurface_GDIText_Update(const PIXEL_LTWH &r) noexcept {
  auto &state = GdiText();
  DIBSECTION dib;
  if (!GetObject(state.surface.img, sizeof(DIBSECTION), &dib)) {
    return false;
  }

  auto *pixels = (static_cast<std::byte *>(dib.dsBm.bmBits) +
                  (r.top * dib.dsBm.bmWidthBytes) +
                  (r.left * (dib.dsBmih.biBitCount / 8)));

  static_assert((GDITEXT_BPP == 32), "Only tested for 32-bit.");
  const auto w = static_cast<size_t>(r.w);
  const auto h = static_cast<size_t>(r.h);
  auto *row_p = pixels;
  for (const auto y : std::views::iota(0u, h)) {
    auto pixels_in_row = std::span(reinterpret_cast<uint32_t *>(row_p), w);
    for (auto &pixel : pixels_in_row) {
      if (pixel != state.color_key) {
        pixel |= state.alpha_mask;
      }
    }
    row_p += dib.dsBm.bmWidthBytes;
  };
  const auto pitch = static_cast<size_t>(dib.dsBm.bmWidthBytes);
  return GrpSurface_Update(SURFACE_ID::TEXT, &r, {pixels, pitch});
}
// -------------------------------
#endif
/// --------

/// Geometry
/// --------

void DrawGeometry(TRIANGLE_PRIMITIVE tp, VERTEX_XY_SPAN<> xys,
                  VERTEX_RGBA_SPAN<> colors) {
#pragma warning(suppress : 26494) // type.5
  SDL_FPoint sdl_vertices[GRP_TRIANGLES_MAX];

  const auto vertex_count = xys.size();
  const auto sdl_colors = HelpColorsFrom(colors);
  const auto indices = INDICES[tp];
  const auto index_count = TriangleIndexCount(vertex_count);
  assert(vertex_count <= std::size(sdl_vertices));
  assert(index_count <= indices.size());
  assert((colors.size() == 1) || (colors.size() == vertex_count));

  // Work around SDL's weird -0.5f offset...
  float offset_x, offset_y;
  SDL_GetRenderScale(*RenderState().renderer, &offset_x, &offset_y);
  offset_x = (1.0f / (2.0f * offset_x));
  offset_y = (1.0f / (2.0f * offset_y));
  auto sdl = std::begin(sdl_vertices);
  for (const auto &game : xys) {
    *(sdl++) = {.x = (game.x + offset_x), .y = (game.y + offset_y)};
  }

  SDL_RenderGeometryRaw(*RenderState().renderer, nullptr, &sdl_vertices[0].x,
                        sizeof(SDL_FPoint), sdl_colors.data(),
                        ((sdl_colors.size() == 1) ? 0 : sizeof(SDL_COLOR)),
                        nullptr, 0, vertex_count, indices.data(), index_count,
                        sizeof(INDEX_TYPE));
}

static void DrawWithAlpha(auto func) {
  SDL_SetRenderDrawBlendMode(*RenderState().renderer, RenderState().alpha_mode);
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b,
                         RenderState().color.a);
  func();
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
  SDL_SetRenderDrawBlendMode(*RenderState().renderer, SDL_BLENDMODE_NONE);
}

void GraphicsGeometry::SetColor(RGB216 col) {
  const auto rgb = col.ToRGB();
  RenderState().color.r = rgb.r;
  RenderState().color.g = rgb.g;
  RenderState().color.b = rgb.b;
  SDL_SetRenderDrawColor(*RenderState().renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
}

void GraphicsGeometry::SetAlphaNorm(uint8_t a) {
  RenderState().color.a = a;
  RenderState().alpha_mode = SDL_BLENDMODE_BLEND;
}

void GraphicsGeometry::SetAlphaOne(void) {
  RenderState().color.a = 0xFF;
  RenderState().alpha_mode = SDL_BLENDMODE_ADD;
}

void GraphicsGeometry::DrawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderLine(*RenderState().renderer, x1, y1, x2, y2);
}

void GraphicsGeometry::DrawBox(int x1, int y1, int x2, int y2) {
  const SDL_FRect rect = {
      .x = static_cast<float>(x1),
      .y = static_cast<float>(y1),
      .w = static_cast<float>(x2 - x1),
      .h = static_cast<float>(y2 - y1),
  };
  SDL_RenderFillRect(*RenderState().renderer, &rect);
}

void GraphicsGeometry::DrawBoxA(int x1, int y1, int x2, int y2) {
  DrawWithAlpha([&] { DrawBox(x1, y1, x2, y2); });
}

void GraphicsGeometry::DrawTriangleFan(VERTEX_XY_SPAN<> xys) {
  DrawTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
}

void GraphicsGeometry::DrawLineStrip(VERTEX_XY_SPAN<> xys) {
  const auto points = HelpFPointsFrom(xys);
  SDL_RenderLines(*RenderState().renderer, points.data(), points.size());
}

void GraphicsGeometry::DrawTriangles(TRIANGLE_PRIMITIVE tp,
                                     VERTEX_XY_SPAN<> xys,
                                     VERTEX_RGBA_SPAN<> colors) {
  if (colors.empty()) {
    const VERTEX_RGBA single = {RenderState().color.r, RenderState().color.g,
                                RenderState().color.b, 0xFF};
    DrawGeometry(tp, xys, std::span(&single, 1));
  } else {
    DrawGeometry(tp, xys, colors);
  }
}

void GraphicsGeometry::DrawTrianglesA(TRIANGLE_PRIMITIVE tp,
                                      VERTEX_XY_SPAN<> xys,
                                      VERTEX_RGBA_SPAN<> colors) {
  DrawWithAlpha([&] {
    if (colors.empty()) {
      const VERTEX_RGBA single = {RenderState().color.r, RenderState().color.g,
                                  RenderState().color.b, RenderState().color.a};
      DrawGeometry(tp, xys, std::span(&single, 1));
    } else {
      DrawGeometry(tp, xys, colors);
    }
  });
}

void GraphicsGeometry::DrawGrdLineEx(int x, int y1, RGB c1, int y2, RGB c2) {
  const auto c1a = c1.WithAlpha(0xFF);
  const auto c2a = c2.WithAlpha(0xFF);
  const VERTEX_XY xys[4] = {
      {static_cast<VERTEX_COORD>(x + 0), static_cast<VERTEX_COORD>(y1)},
      {static_cast<VERTEX_COORD>(x + 0), static_cast<VERTEX_COORD>(y2)},
      {static_cast<VERTEX_COORD>(x + 1), static_cast<VERTEX_COORD>(y1)},
      {static_cast<VERTEX_COORD>(x + 1), static_cast<VERTEX_COORD>(y2)},
  };
  const VERTEX_RGBA colors[4] = {c1a, c2a, c1a, c2a};
  DrawGeometry(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
}

/// --------

/// Software rendering with pixel access
/// ------------------------------------

static SDL_Texture *EnsureSoftwareTexture(void) {
  if (RenderState().software_texture) {
    return RenderState().software_texture;
  }
  RenderState().software_texture = SDL_CreateTexture(
      RenderState().primary_renderer, RenderState().software_surface->format,
      SDL_TEXTUREACCESS_STREAMING, RenderState().software_surface->w,
      RenderState().software_surface->h);
  if (!RenderState().software_texture) {
    logging::SdlError(LOG_CAT, "Error creating software rendering texture");
    DestroySoftwareRenderer();
    return nullptr;
  }
  TexturePostInit(*RenderState().software_texture,
                  RenderState().primary_renderer);
  return RenderState().software_texture;
}

bool GrpBackend_PixelAccessStart(void) {
  if (RenderState().software_renderer) {
    return true;
  }
  RenderState().software_renderer =
      SDL_CreateSoftwareRenderer(RenderState().software_surface);
  if (!RenderState().software_renderer) {
    logging::SdlError(LOG_CAT, "Error creating software renderer");
    return DestroySoftwareRenderer();
  }
  SwitchActiveRenderer(&RenderState().software_renderer);
  return (EnsureSoftwareTexture() != nullptr);
}

bool GrpBackend_PixelAccessEnd(void) {
  if (!RenderState().software_renderer) {
    return true;
  }
  SwitchActiveRenderer(&RenderState().primary_renderer);
  DestroySoftwareRenderer();
  return true;
}

std::tuple<std::byte *, size_t> GrpBackend_PixelAccessLock(void) {
  // Necessary in SDL 3!
  SDL_FlushRenderer(RenderState().software_renderer);

  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    if (!SDL_LockSurface(RenderState().software_surface)) {
      logging::SdlError(LOG_CAT, "Error locking CPU backbuffer");
      return {nullptr, 0};
    }
  }
  auto *pixels =
      static_cast<std::byte *>(RenderState().software_surface->pixels);
  return {pixels, RenderState().software_surface->pitch};
}

void GrpBackend_PixelAccessUnlock(void) {
  if (SDL_MUSTLOCK(RenderState().software_surface)) {
    SDL_UnlockSurface(RenderState().software_surface);
  }
}
/// ------------------------------------
