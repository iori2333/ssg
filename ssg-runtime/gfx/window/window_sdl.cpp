///
/// SDL window creation
///

#include <cstdint>
#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <type_traits>
#include <utility>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include "window.h"
#include "window_sdl.h"

#include "gfx/core/constants.h"
#include "gfx/graphics.h"
#include "gfx/graphics_system.h"
#include "sys/log.h"
#include "util/sdl_resource.h"

constexpr auto kLogCat = logging::Channel::Graphics;

// At least on Windows, SDL 3's default graphics API (Direct3D 11) also appears
// to be the most performant choice:
//
// 	https://rec98.nmlgc.net/blog/2025-04-09#sdl3-2025-04-09
//
// Hence, Windows builds also get pixel-perfect line rendering compared to
// pbg's original build by default:
//
// 	https://rec98.nmlgc.net/blog/2024-10-22#lines-2024-10-22
constexpr const char *kSdlDefaultApi = nullptr;

namespace {

std::string_view SdlRawRenderDriverName(int id) {
  const auto *name = SDL_GetRenderDriver(id);
  return name != nullptr ? std::string_view{name} : std::string_view{};
}

gfx::WindowState &State() { return gfx::ActiveGraphics().window; }

} // namespace

void SdlWindowRememberPosition(std::pair<int, int> position) {
  State().topleft_before_fullscreen = position;
}

std::pair<int, int> SdlWindowPosition(SDL_Window *window) {
  int left{};
  int top{};
  SDL_GetWindowPosition(window, &left, &top);
  return std::make_pair(left, top);
}

SDL_DisplayID SdlDisplayForWindow() {
  if (State().window == nullptr) {
    return SDL_GetPrimaryDisplay();
  }
  const auto ret = SDL_GetDisplayForWindow(State().window);
  if (ret == 0) {
    return SDL_GetPrimaryDisplay();
  }
  return ret;
}

PixelPoint SdlGraphicsDisplaySize(bool fullscreen) {
  SDL_Rect rect{};
  const auto display_i = SdlDisplayForWindow();
  if (fullscreen) {
    const auto *display_mode = SDL_GetDesktopDisplayMode(display_i);
    if (display_mode == nullptr) {
      logging::SdlError(kLogCat, "Error retrieving display size");
      return kGameResolution;
    }
    return {.x = display_mode->w, .y = display_mode->h};
  }

  if (!SDL_GetDisplayUsableBounds(display_i, &rect)) {
    logging::SdlError(kLogCat, "Error retrieving display size");
    return kGameResolution;
  }
  return {.x = rect.w, .y = rect.h};
}

// Don't do a ZUN.
// (https://github.com/thpatch/thcrap/commit/71c1dcab690f85653cbc9a06c7c55)
namespace {
SDL_Rect ClampWindowRect(SDL_Rect window_rect) {
  // SDL_GetDisplayForRect() returns the *closest* display, so we need to
  // manually clamp the window to its bounds.
  const auto display_i = SDL_GetDisplayForRect(&window_rect);
  if (display_i == 0) {
    return window_rect;
  }
  SDL_Rect display_rect{};
  if (static_cast<int>(SDL_GetDisplayUsableBounds(display_i, &display_rect)) !=
      0) {
    return window_rect;
  }

  const auto clamp_start = [](int win_start, int win_extent, int disp_start,
                              int disp_extent) {
    if (SDL_WINDOWPOS_ISCENTERED(win_start) ||
        SDL_WINDOWPOS_ISUNDEFINED(win_start)) {
      return win_start;
    }
    const auto win_end = (win_start + win_extent);
    const auto disp_end = (disp_start + disp_extent);
    if (win_end < disp_start) {
      return disp_start;
    }
    if (win_end > disp_end) {
      return (disp_end - win_extent);
    }
    return win_start;
  };

  window_rect.x =
      clamp_start(window_rect.x, window_rect.w, display_rect.x, display_rect.w);
  window_rect.y =
      clamp_start(window_rect.y, window_rect.h, display_rect.y, display_rect.h);
  return window_rect;
}
} // namespace

std::optional<WindowFullscreenState>
SdlSetFullscreen(SDL_Window *window, WindowFullscreenState state) {
  for (;;) {
    if (state.enabled && state.exclusive) {
#pragma warning(suppress : 26494) // type.5
      SDL_DisplayMode mode;

      constexpr auto rate = (1000.0F / kFrameTimeTarget);
      if (!SDL_GetClosestFullscreenDisplayMode(
              SdlDisplayForWindow(), kGameResolution.x, kGameResolution.y, rate,
              false, &mode)) {
        logging::SdlError(kLogCat,
                          "Could not find a display mode for exclusive "
                          "fullscreen, falling back on borderless");
        state.exclusive = false;
        continue;
      }
      SDL_SetWindowFullscreenMode(window, &mode);
    } else {
      SDL_SetWindowFullscreenMode(window, nullptr);
    }
    if (!SDL_SetWindowFullscreen(window, state.enabled)) {
      logging::SdlError(kLogCat, "Error changing display mode");
      return std::nullopt;
    }
    return state;
  }
}

int SdlValidateRenderDriver(std::string_view hint) {
  for (const auto id : std::views::iota(0, SDL_GetNumRenderDrivers())) {
    if (SdlRawRenderDriverName(id) == hint) {
      return id;
    }
  }
  const auto *default_driver =
      ((kSdlDefaultApi != nullptr) ? kSdlDefaultApi
                                   : SdlRawRenderDriverName(0).data());
  logging::Warning(
      kLogCat,
      "Unsupported renderer \"{}\" specified in " SDL_HINT_RENDER_DRIVER
      " hint; falling back to {} default ({})",
      hint, ((kSdlDefaultApi != nullptr) ? "the" : "SDL's"), default_driver);
  SDL_UnsetEnvironmentVariable(SDL_GetEnvironment(), SDL_HINT_RENDER_DRIVER);

  // If this succeeds, the hint came from SDL, not the environment.
  if (SDL_GetHint(SDL_HINT_RENDER_DRIVER) != nullptr) {
    SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, nullptr, SDL_HINT_OVERRIDE);
  }
  return -1;
}

std::string_view SdlRenderDriverName(int id) {
  const auto driver_count = SDL_GetNumRenderDrivers();
  if (id >= 0 && id < driver_count) {
    return SdlRawRenderDriverName(id);
  }
  if (id >= 0) {
    logging::Warning(
        kLogCat, "Renderer index {} is out of range; using the default", id);
    id = -1;
  }

  const auto *hint = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
  if (hint == nullptr) {
    hint = kSdlDefaultApi;
  }
  if ((hint == nullptr) || (hint[0] == '\0')) {
    // SDL tries to initialize drivers in order.
    return SdlRawRenderDriverName(0);
  }
  id = SdlValidateRenderDriver(hint);
  if (id < 0) {
    if constexpr (kSdlDefaultApi != nullptr) {
      return kSdlDefaultApi;
    }
    return SdlRawRenderDriverName(0);
  }
  return SdlRawRenderDriverName(id);
}

SDL_Window *SdlWindow() { return State().window; }

std::optional<GraphicsParams> SdlWindowCreate(GraphicsParams params) {
  if (State().window != nullptr) {
    logging::Critical(kLogCat,
                      "Cannot create an SDL window while one already exists");
    return std::nullopt;
  }

  // The driver/API parameter takes precedence over the environment variable,
  // which is a bad idea in case the user is stuck on an API that might
  // initialize successfully but refuses to render properly. Let's reverse
  // that behavior to provide a way of overriding [params.render_driver] with a
  // specific renderer.
  // Note that we need to directly access the environment variable because
  // SDL's hint system does not indicate where the hint came from. Also, we
  // only want this override to apply to the first init call – if it didn't,
  // the user couldn't specify a different API for subsequent init calls.
  auto *env = SDL_GetEnvironment();
  const auto *driver_env_ptr =
      (SDL_GetEnvironmentVariable(env, SDL_HINT_RENDER_DRIVER));
  SDL_UnsetEnvironmentVariable(env, SDL_HINT_RENDER_DRIVER);

  // We can actually get empty strings on SDL 2 here!
  if ((driver_env_ptr != nullptr) && (driver_env_ptr[0] != '\0')) {
    params.render_driver = SdlValidateRenderDriver(driver_env_ptr);
  } else if (kSdlDefaultApi != nullptr) {
    if ((params.render_driver < 0) &&
        (SDL_GetHint(SDL_HINT_RENDER_DRIVER) == nullptr)) {
      SDL_SetHint(SDL_HINT_RENDER_DRIVER, kSdlDefaultApi);
    }
  }

  // Set the necessary window flags for certain APIs to avoid
  // SDL_CreateRenderer()'s janky closing and reopening of the window with
  // the correct flags.
  const auto name = SdlRenderDriverName(params.render_driver);
  uint32_t flags = 0;
  if (name.starts_with("opengl")) {
    flags |= SDL_WINDOW_OPENGL;
    SDL_GL_ResetAttributes();

    // SDL_GL_ResetAttributes() also resets the essential profile mask and
    // version selection attributes, but chooses the target OpenGL version
    // via a hardcoded #ifdef priority list that prefers regular OpenGL
    // over ES 2 over ES 1. If the user requested any of the ES versions,
    // SDL_CreateRenderer() would still recreate the window because the
    // `SDL_GL_CONTEXT_PROFILE_MASK` got set to regular/non-ES OpenGL.
    // So, we're forced to specify the correct flags ourselves after all.
    const auto [maj, min] = ([name]() -> std::pair<int, int> {
      if (name.starts_with("opengles")) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                            SDL_GL_CONTEXT_PROFILE_ES);
        if (name == "opengles2") {
          return {2, 0};
        }
        return {1, 1};
      }
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, 0);
      return {2, 1};
    })();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, maj);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, min);
  }

  if ((params.window_left != kGraphicsTopleftUndefined) ||
      (params.window_top != kGraphicsTopleftUndefined)) {
    State().topleft_before_fullscreen = {params.window_left, params.window_top};
  }

  const auto real_pos = [](int pos) -> int {
    return ((pos == kGraphicsTopleftUndefined) ? SDL_WINDOWPOS_CENTERED : pos);
  };

  const auto res = params.ScaledRes(SdlGraphicsDisplaySize(params.fullscreen));
  const WindowFullscreenState fullscreen = {
      .enabled = params.fullscreen,
      .exclusive = params.exclusive_fullscreen,
  };
  SDL_Rect rect = {
      .x = real_pos(params.window_left),
      .y = real_pos(params.window_top),
      .w = res.x,
      .h = res.y,
  };
  if (!fullscreen.enabled) {
    rect = ClampWindowRect(rect);
  }

  const SDL_PropertiesID props = SDL_CreateProperties();
  SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING,
                        std::string(kGameTitle).c_str());
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, rect.x);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, rect.y);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, rect.w);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, rect.h);
  SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, flags);

  // SDL 3 can only enter exclusive fullscreen after the window has been
  // created, so...
  SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);

  State().window = SDL_CreateWindowWithProperties(props);
  SDL_DestroyProperties(props);
  if (State().window == nullptr) {
    logging::SdlError(kLogCat, "Error creating SDL window");
    return std::nullopt;
  }
  const auto maybe_fs_actual = SdlSetFullscreen(State().window, fullscreen);
  if (!maybe_fs_actual) {
    State().window.Reset();
    return std::nullopt;
  }
  const auto actual = *maybe_fs_actual;
  params.fullscreen = actual.enabled;
  params.exclusive_fullscreen = actual.exclusive;
  SDL_ShowWindow(State().window);
  return params;
}

void SdlWindowCleanup() { State().window.Reset(); }

std::optional<std::pair<int, int>> WindowPosition() {
  // A fullscreen window is always positioned at (0, 0), and we don't want to
  // override any previous windowed position.
  if ((State().window == nullptr) ||
      ((SDL_GetWindowFlags(State().window) & SDL_WINDOW_FULLSCREEN) != 0U)) {
    return State().topleft_before_fullscreen;
  }
  return SdlWindowPosition(State().window);
}

int WindowRun(const std::function<void()> &poll_input,
              const std::function<bool()> &run_frame) {
  bool quit = false;
  uint64_t ticks_last = 0;

  while (!quit) {
    // Read input events first to remove them from the queue
    SDL_PumpEvents();
    poll_input();

    SDL_Event event;
    while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_EVENT_FIRST,
                          SDL_EVENT_LAST) == 1) {
      if (event.type == SDL_EVENT_QUIT) {
        return 0;
      }
    }

    const auto ticks_start = SDL_GetTicks();
    if ((FrameRateDivisor() == 0) ||
        ((ticks_start - ticks_last) >= kFrameTimeTarget)) {
      quit = !run_frame();
      if (FrameRateDivisor() != 0) {
        // Since SDL_Delay() works at not-even-exact millisecond
        // granularity, we subtract 1 and spin for the last
        // millisecond to ensure that we hit the exact frame
        // boundary.
        const auto ticks_frame = (SDL_GetTicks() - ticks_start);
        if (ticks_frame < (kFrameTimeTarget - 1)) {
          SDL_Delay((kFrameTimeTarget - 1) - ticks_frame);
        }
        ticks_last = ticks_start;
      }
    }
  }
  return 0;
}
