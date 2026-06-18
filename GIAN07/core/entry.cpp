///
/// Entry - Generic, cross-platform subsystem initialization and cleanup
///

#include "entry.h"
#include "config.h"
#include "game/bgm.h"
#include "game/debug.h"
#include "game/frame.h"
#include "game/input.h"
#include "game/snd.h"
#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
#include "loader.h"
#include "obj/platform_constants.h"
#include "platform/graphics_backend.h"
#include "platform/input.h"
#include "platform/path.h"
#include "platform/text_backend.h"
#include "platform/window_backend.h"
#include <SDL3/SDL_filesystem.h>
#include <chrono>
#include <filesystem>

// Screenshots
// -----------

const uint8_t &Grp_ScreenshotEffort = ConfigDat.ScreenshotEffort.v;
// -----------

// Volume controls
// ---------------

const VOLUME &Mid_Volume = ConfigDat.BGMVolume.v;
const VOLUME &Snd_VolumeBGM = ConfigDat.BGMVolume.v;
const VOLUME &Snd_VolumeSE = ConfigDat.SEVolume.v;
// ---------------

// MUSIC.DAT loaders
// -----------------

bool (*const BGM_MidLoadOriginal)(unsigned int id) = LoadMusic;
bool (*const BGM_MidLoadBuffer)(BYTE_BUFFER_OWNED) = LoadMIDIBuffer;
bool (*const BGM_MidLoadByHash)(const HASH &hash) = LoadMusicByHash;
// -----------------

// Pad bindings
// ------------

static constexpr std::array<INPUT_PAD_BINDING, 4> PadBindings = {{
    {ConfigDat.PadTama.v, KEY_TAMA},
    {ConfigDat.PadBomb.v, KEY_BOMB},
    {ConfigDat.PadShift.v, KEY_SHIFT},
    {ConfigDat.PadCancel.v, KEY_ESC},
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
  ConfigLoad();
  Grp_FPSDivisor = ConfigDat.FPSDivisor.v;
  ConfigDat.MidFlags.v = Mid_SetFlags(ConfigDat.MidFlags.v);

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
  if ((ConfigDat.SoundFlags.v & SNDF_BGM_ENABLE) != 0) {
    BGM_Init();
  }
  if (!BGM_PackSet(ConfigDat.BGMPack)) {
    ConfigDat.BGMPack.clear();
  }
  BGM_SetGainApply((ConfigDat.SoundFlags.v & SNDF_BGM_NOT_VOL_NORM) == 0);
  Grp_ScreenshotSetPrefix("screenshots/");
  LoaderInit();
  return true;
}

void XCleanup() {
  LoaderCleanup();
  ConfigSave();
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
      ReloadGraph();
    }

    // TODO: Assumes that 8-bit mode only ever calls this function within
    // the main menu. If we ever add hotkeys to toggle between 8-bit and
    // 16-bit modes (https://github.com/nmlgc/ssg/issues/72), this has to
    // be solved more cleanly as part of the backend.
    GrpSurface_PaletteApplyToBackend(SURFACE_ID::TITLE);
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
      ConfigDat.FPSDivisor.v = Grp_FPSDivisor = 0;
    } else {
      ConfigDat.FPSDivisor.v = Grp_FPSDivisor = fps_divisor_prev;
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
