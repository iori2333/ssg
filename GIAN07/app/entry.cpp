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

static ConfigData g_config;

const uint8_t &Grp_ScreenshotEffort = g_config.graphics.screenshot_effort;

const VOLUME &Mid_Volume = g_config.audio.bgm_volume;
const VOLUME &Snd_VolumeBGM = g_config.audio.bgm_volume;
const VOLUME &Snd_VolumeSE = g_config.audio.se_volume;

static std::array<INPUT_PAD_BINDING, 4> MakePadBindings() {
  return {{
      {g_config.input.pad_tama, KEY_TAMA},
      {g_config.input.pad_bomb, KEY_BOMB},
      {g_config.input.pad_shift, KEY_SHIFT},
      {g_config.input.pad_cancel, KEY_ESC},
  }};
}

static std::array<INPUT_PAD_BINDING, 4> PadBindings = MakePadBindings();
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
  g_config.Load();
  Grp_FPSDivisor = g_config.graphics.fps_divisor;
  Mid_SetFlags(g_config.audio.fix_sysex_bugs ? MID_FLAGS::FIX_SYSEX_BUGS
                                             : MID_FLAGS::NONE);

  // Config-dependent initialization
  if (!GrpBackend_Enum()) {
    return false;
  }

  // Initialize graphics
  const auto maybe_params = Grp_InitOrFallback(g_config.graphics.GraphicsParams());
  if (!maybe_params) {
    return false;
  }
  g_config.graphics.GraphicsParamsApply(maybe_params.value().live);
  GrpBackend_SetClip(GRP_RES_RECT);

  // Accept keyboard (JoyPad) input
  Key_Start();

  // Initialize BGM
  if (g_config.audio.bgm_enabled) {
    BGM_Init(g_config.audio.soundfont);
  }
  if (!track_mgr.PackSet(g_config.audio.bgm_pack)) {
    g_config.audio.bgm_pack.clear();
  }
  BGM_SetGainApply(g_config.audio.bgm_vol_norm);
  GameFlow.ctx.game.game_config_ = &g_config.game;
  GameFlow.ctx.game_cfg = &g_config.game;
  GameFlow.ctx.graphics_cfg = &g_config.graphics;
  GameFlow.ctx.audio_cfg = &g_config.audio;
  GameFlow.ctx.input_cfg = &g_config.input;
  GameFlow.ctx.debug_cfg = &g_config.debug;

  GameFlow.ctx.save_config = [&] { SaveConfigFile(g_config); };
  GameFlow.ctx.cfg = &g_config;

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
  SaveConfigFile(g_config);
  TextBackend_Cleanup();
  GrpBackend_Cleanup();
  BGM_Cleanup();
  Snd_Cleanup();
  Key_End();
}

bool GameFrame() {
#ifdef SUPPORT_GRP_WINDOWED
  if ((SystemKey_Data & SYSKEY_GRP_FULLSCREEN) != 0) {
    XGrpTryCycleDisp(g_config.graphics);
  }
#endif
#ifdef SUPPORT_GRP_SCALING
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_UP) != 0) {
    XGrpTryCycleScale(g_config.graphics, +1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_DOWN) != 0) {
    XGrpTryCycleScale(g_config.graphics, -1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_MODE) != 0) {
    XGrpTryCycleScMode(g_config.graphics);
  }
#endif
  if ((SystemKey_Data & SYSKEY_GRP_TURBO) != 0) {
    static decltype(Grp_FPSDivisor) fps_divisor_prev =
        ((Grp_FPSDivisor != 0) ? Grp_FPSDivisor : 1);
    if (Grp_FPSDivisor != 0) {
      fps_divisor_prev = Grp_FPSDivisor;
      g_config.graphics.fps_divisor = Grp_FPSDivisor = 0;
    } else {
      g_config.graphics.fps_divisor = Grp_FPSDivisor = fps_divisor_prev;
    }
  }
#ifdef SUPPORT_GRP_API
  if ((SystemKey_Data & SYSKEY_GRP_API) != 0) {
    XGrpTry(g_config.graphics, [](auto &params) {
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
