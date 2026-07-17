///
/// Entry - Generic, cross-platform subsystem initialization and cleanup
///

#include <chrono>
#include <filesystem>

#include <SDL3/SDL_filesystem.h>

#include "config.h"
#include "entry.h"

#include "audio/bgm.h"
#include "audio/snd.h"
#include "data/gfx_manager.h"
#include "data/init.h"
#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
#include "gfx/frame.h"
#include "gfx/graphics_backend.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "sys/path.h"
#include "util/debug.h"

// Screenshots
// -----------

const uint8_t &Grp_ScreenshotEffort = ConfigDat.screenshot_effort;
// -----------

// Volume controls
// ---------------

const VOLUME &Mid_Volume = ConfigDat.bgm_volume;
const VOLUME &Snd_VolumeBGM = ConfigDat.bgm_volume;
const VOLUME &Snd_VolumeSE = ConfigDat.se_volume;
// ---------------

// Pad bindings
// ------------

static constexpr std::array<INPUT_PAD_BINDING, 4> PadBindings = {{
    {ConfigDat.pad_tama, KEY_TAMA},
    {ConfigDat.pad_bomb, KEY_BOMB},
    {ConfigDat.pad_shift, KEY_SHIFT},
    {ConfigDat.pad_cancel, KEY_ESC},
}};
std::span<const INPUT_PAD_BINDING> Key_PadBindings = PadBindings;
// ------------

bool XInit() {
  const auto path_data = PathForData();
  std::error_code ec;
  std::filesystem::current_path(path_data, ec);
  if (ec) {
    return false;
  }

#ifdef WIN32
  // User data and game data directories are one and the same on Windows,
  // and we already ship with the skeleton in place.
  // The release archive might have added some 0-byte binaries that we need
  // to get rid of, though.
  SDL_EnumerateDirectory(
      path_data.data(),
      [](void *, const char *, const char *basename_p) {
        const auto *ext = SDL_strrchr(basename_p, '.');
        if (!ext) {
          return SDL_ENUM_CONTINUE;
        }
        if (SDL_strcasecmp(ext, ".exe") && SDL_strcasecmp(ext, ".dll")) {
          return SDL_ENUM_CONTINUE;
        }
        SDL_PathInfo info;
        if (!SDL_GetPathInfo(basename_p, &info)) {
          return SDL_ENUM_CONTINUE;
        }
        if ((info.type == SDL_PATHTYPE_FILE) && (info.size == 0)) {
          SDL_RemovePath(basename_p);
        }
        return SDL_ENUM_CONTINUE;
      },
      nullptr);
#elifndef PATH_SKELETON
#pragma message(                                                               \
    "No user data skeleton directory defined. This does not matter for development builds, but package builds should set the environment variable `PATH_SKELETON` accordingly.")
#else
  using copy_opts = std::filesystem::copy_options;

  const auto opts = (copy_opts::skip_existing | copy_opts::recursive);
  std::filesystem::copy(PATH_SKELETON, path_data, opts, ec);
#endif

  DebugSetup();

  // Load config
  ConfigDat.Load();
  Grp_FPSDivisor = ConfigDat.fps_divisor;
  ConfigDat.midi_flags = Mid_SetFlags(ConfigDat.midi_flags);

  // Config-dependent initialization
  if (!GrpBackend_Enum()) {
    return false;
  }

  // Initialize graphics
  const auto maybe_params = Grp_InitOrFallback(ConfigDat.GraphicsParams());
  if (!maybe_params) {
    return false;
  }
  ConfigDat.GraphicsParamsApply(maybe_params.value().live);
  GrpBackend_SetClip(GRP_RES_RECT);

  // Accept keyboard (JoyPad) input
  Key_Start();

  // Initialize BGM
  if (ConfigDat.bgm_enabled) {
    BGM_Init();
  }
  if (!BGM_PackSet(ConfigDat.bgm_pack)) {
    ConfigDat.bgm_pack.clear();
  }
  BGM_SetGainApply(ConfigDat.bgm_vol_norm);
  Grp_ScreenshotSetPrefix("screenshots/");
  DataInit();
  return true;
}

void XCleanup() {
  DataCleanup();
  ConfigDat.Save();
  TextBackend_Cleanup();
  GrpBackend_Cleanup();
  BGM_Cleanup();
  Snd_Cleanup();
  Key_End();
}

void XGrpTry(const GRAPHICS_PARAMS &prev, GRAPHICS_PARAMS &params) {
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
    ConfigDat.GraphicsParamsApply(result.live);
    if (result.reload_surfaces) {
      gfx.ReloadStage();
    }
  }
}

void XGrpTryCycleScale(int_fast8_t delta, bool include_max) {
  XGrpTry([&](auto &params) {
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

void XGrpTryCycleDisp() {
  XGrpTry(
      [](auto &params) { params.flags ^= GRAPHICS_PARAM_FLAGS::FULLSCREEN; });
}

void XGrpTryCycleScMode() {
  XGrpTry([](auto &params) {
    params.flags ^= GRAPHICS_PARAM_FLAGS::SCALE_GEOMETRY;
  });
}

bool GameFrame() {
#ifdef SUPPORT_GRP_WINDOWED
  if ((SystemKey_Data & SYSKEY_GRP_FULLSCREEN) != 0) {
    XGrpTryCycleDisp();
  }
#endif
#ifdef SUPPORT_GRP_SCALING
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_UP) != 0) {
    XGrpTryCycleScale(+1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_DOWN) != 0) {
    XGrpTryCycleScale(-1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_MODE) != 0) {
    XGrpTryCycleScMode();
  }
#endif
  if ((SystemKey_Data & SYSKEY_GRP_TURBO) != 0) {
    static decltype(Grp_FPSDivisor) fps_divisor_prev =
        ((Grp_FPSDivisor != 0) ? Grp_FPSDivisor : 1);
    if (Grp_FPSDivisor != 0) {
      fps_divisor_prev = Grp_FPSDivisor;
      ConfigDat.fps_divisor = Grp_FPSDivisor = 0;
    } else {
      ConfigDat.fps_divisor = Grp_FPSDivisor = fps_divisor_prev;
    }
  }
#ifdef SUPPORT_GRP_API
  if ((SystemKey_Data & SYSKEY_GRP_API) != 0) {
    XGrpTry([](auto &params) {
      params.api = ((params.api + 1) % GrpBackend_APICount());
    });
  }
#endif

  // Strictly superior to waiting [CWIN_KEYWAIT] frames: We won't get a key
  // release scancode if we recreate the window on a switch into exclusive
  // fullscreen, and we save a dependency on `WindowSys.h`.
  SystemKey_Data &=
      ~(SYSKEY_GRP_FULLSCREEN | SYSKEY_GRP_SCALE_UP | SYSKEY_GRP_SCALE_DOWN |
        SYSKEY_GRP_SCALE_MODE | SYSKEY_GRP_TURBO | SYSKEY_GRP_API);

  bool quit = false;
  GameFlow.game_main(quit);
  return !quit;
}
