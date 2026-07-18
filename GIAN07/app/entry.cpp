///
/// Entry - Generic, cross-platform subsystem initialization and cleanup
///

#include <chrono>
#include <filesystem>

#include <SDL3/SDL_filesystem.h>

#include "core/config.h"
#include "core/graphics_settings.h"
#include "entry.h"

#include "audio/bgm.h"
#include "track_manager/track_manager.h"
#include "audio/snd.h"
#include "core/gian.h"
#include "data/gfx_manager.h"
#include "data/init.h"
#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
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
    BGM_Init(ConfigDat.soundfont);
  }
  if (!track_mgr.PackSet(ConfigDat.bgm_pack)) {
    ConfigDat.bgm_pack.clear();
  }
  BGM_SetGainApply(ConfigDat.bgm_vol_norm);
  Grp_ScreenshotSetPrefix("screenshots/");
  const auto err = DataInit();
  if (err.has_value()) {
    GameFlow.game_main = [](bool &quit) { quit = true; };
    GameFlow.current_state = GameState::External;
  } else {
    SProjectInit();
  }
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
