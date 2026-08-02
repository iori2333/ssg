///
/// SDL window creation
///

#include <cstdint>
#include <functional>
#include <optional>
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

#include "constants.h"
#include "graphics.h"
#include "graphics_backend.h"
#include "window_backend.h"
#include "window_sdl.h"

#include "sys/log.h"

constexpr auto kLogCat = logging::Channel::Graphics;

namespace {

struct WindowState {
  SDL_Window *window = nullptr;
  std::optional<std::pair<int, int>> topleft_before_fullscreen;
};

WindowState &State() {
  static WindowState state;
  return state;
}

} // namespace

void WindowBackendRememberTopleft(std::pair<int, int> position) {
  State().topleft_before_fullscreen = position;
}

std::pair<int, int> HelpGetWindowPosition(SDL_Window *window) {
  int left{};
  int top{};
  SDL_GetWindowPosition(window, &left, &top);
  return std::make_pair(left, top);
}

SDL_DisplayID HelpGetDisplayForWindow() {
  if (State().window == nullptr) {
    return SDL_GetPrimaryDisplay();
  }
  const auto ret = SDL_GetDisplayForWindow(State().window);
  if (ret == 0) {
    return SDL_GetPrimaryDisplay();
  }
  return ret;
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

std::optional<GraphicsFullscreenFlags>
HelpSetFullscreenMode(SDL_Window *window, GraphicsFullscreenFlags fs) {
  for (;;) {
    if (fs.fullscreen && fs.exclusive) {
#pragma warning(suppress : 26494) // type.5
      SDL_DisplayMode mode;

      constexpr auto rate = (1000.0F / kFrameTimeTarget);
      if (!SDL_GetClosestFullscreenDisplayMode(
              HelpGetDisplayForWindow(), kGameResolution.w, kGameResolution.h,
              rate, false, &mode)) {
        logging::SdlError(kLogCat,
                          "Could not find a display mode for exclusive "
                          "fullscreen, falling back on borderless");
        fs.exclusive = false;
        continue;
      }
      SDL_SetWindowFullscreenMode(window, &mode);
    } else {
      SDL_SetWindowFullscreenMode(window, nullptr);
    }
    if (!SDL_SetWindowFullscreen(window, fs.fullscreen)) {
      logging::SdlError(kLogCat, "Error changing display mode");
      return std::nullopt;
    }
    return fs;
  }
}

int WindowBackendValidateRenderDriver(std::string_view hint) {
  const auto id = GraphicsBackendAPIID(hint);
  if (id >= 0) {
    return id;
  }
  const auto *default_driver =
      ((kSdlDefaultApi != nullptr) ? kSdlDefaultApi
                                   : GraphicsBackendAPIString(0).data());
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

std::string_view WindowBackendSDLRendererName(int id) {
  const auto driver_count = SDL_GetNumRenderDrivers();
  if (id >= 0 && id < driver_count) {
    return GraphicsBackendAPIString(id);
  }
  if (id >= 0) {
    logging::Warning(kLogCat,
                     "Renderer index {} is out of range; using the default",
                     id);
    id = -1;
  }

  const auto *hint = SDL_GetHint(SDL_HINT_RENDER_DRIVER);
  if (hint == nullptr) {
    hint = kSdlDefaultApi;
  }
  if ((hint == nullptr) || (hint[0] == '\0')) {
    // SDL tries to initialize drivers in order.
    return GraphicsBackendAPIString(0);
  }
  id = WindowBackendValidateRenderDriver(hint);
  if (id < 0) {
    if constexpr (kSdlDefaultApi != nullptr) {
      return kSdlDefaultApi;
    }
    return GraphicsBackendAPIString(0);
  }
  return GraphicsBackendAPIString(id);
}

SDL_Window *WindowBackendSDL() { return State().window; }

std::optional<GraphicsParams> WindowBackendCreate(GraphicsParams params) {
  if (State().window != nullptr) {
    logging::Critical(kLogCat,
                      "Cannot create an SDL window while one already exists");
    return std::nullopt;
  }

  // The driver/API parameter takes precedence over the environment variable,
  // which is a bad idea in case the user is stuck on an API that might
  // initialize successfully but refuses to render properly. Let's reverse
  // that behavior to provide a way of overriding [params.api] with a
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
    params.api = WindowBackendValidateRenderDriver(driver_env_ptr);
  } else if (kSdlDefaultApi != nullptr) {
    if ((params.api < 0) && (SDL_GetHint(SDL_HINT_RENDER_DRIVER) == nullptr)) {
      SDL_SetHint(SDL_HINT_RENDER_DRIVER, kSdlDefaultApi);
    }
  }

  // Set the necessary window flags for certain APIs to avoid
  // SDL_CreateRenderer()'s janky closing and reopening of the window with
  // the correct flags.
  const auto name = WindowBackendSDLRendererName(params.api);
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

  if ((params.left != 0) || (params.top != 0)) {
    State().topleft_before_fullscreen = {params.left, params.top};
  }

  const auto real_pos = [](int pos) -> int {
    return ((pos == kGraphicsTopleftUndefined) ? SDL_WINDOWPOS_CENTERED : pos);
  };

  const auto res = params.ScaledRes();
  const auto fs = params.FullscreenFlags();
  SDL_Rect rect = {
      .x = real_pos(params.left),
      .y = real_pos(params.top),
      .w = res.w,
      .h = res.h,
  };
  if (!fs.fullscreen) {
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
  const auto maybe_fs_actual = HelpSetFullscreenMode(State().window, fs);
  if (!maybe_fs_actual) {
    SDL_DestroyWindow(State().window);
    return std::nullopt;
  }
  const auto fs_actual = maybe_fs_actual.value();
  using F = GraphicsParamFlags;
  params.SetFlag(F::Fullscreen,
                 static_cast<std::underlying_type_t<GraphicsParamFlags>>(
                     fs_actual.fullscreen));
  params.SetFlag(F::FullscreenExclusive,
                 static_cast<std::underlying_type_t<GraphicsParamFlags>>(
                     fs_actual.exclusive));
  SDL_ShowWindow(State().window);
  return params;
}

void WindowBackendCleanup() {
  if (State().window != nullptr) {
    SDL_DestroyWindow(State().window);
    State().window = nullptr;
  }
}

std::optional<std::pair<int, int>> WindowBackendTopleft() {
  // A fullscreen window is always positioned at (0, 0), and we don't want to
  // override any previous windowed position.
  if ((State().window == nullptr) ||
      ((SDL_GetWindowFlags(State().window) & SDL_WINDOW_FULLSCREEN) != 0U)) {
    return State().topleft_before_fullscreen;
  }
  return HelpGetWindowPosition(State().window);
}

int WindowBackendRun(const std::function<void()> &input_func,
                     const std::function<bool()> &frame_func) {
  bool quit = false;
  uint64_t ticks_last = 0;

  while (!quit) {
    // Read input events first to remove them from the queue
    SDL_PumpEvents();
    input_func();

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
      quit = !frame_func();
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
