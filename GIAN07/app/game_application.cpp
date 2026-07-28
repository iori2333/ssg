/// Top-level application lifetime and composition root.

#include <array>
#include <filesystem>
#include <memory>

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_messagebox.h>

#include "game_application.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/snd.h"
#include "data/game_data.h"
#include "gameflow/game_flow.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/window_backend.h"
#include "platform/text_backend.h"
#include "sys/input.h"
#include "sys/path.h"
#include "util/debug.h"

namespace {

bool PrepareWorkingDirectory() {
  const auto data_path = PathForData();
  std::error_code error;
  std::filesystem::current_path(data_path, error);
  if (error) {
    return false;
  }
  return true;
}

void ApplyPadBindings(const InputConfig &config) {
  const std::array bindings = {
      INPUT_PAD_BINDING{config.pad_tama, KEY_TAMA},
      INPUT_PAD_BINDING{config.pad_bomb, KEY_BOMB},
      INPUT_PAD_BINDING{config.pad_shift, KEY_SHIFT},
      INPUT_PAD_BINDING{config.pad_cancel, KEY_ESC},
  };
  Key_SetPadBindings(bindings);
}

} // namespace

GameApplication::GameApplication() = default;

GameApplication::~GameApplication() { Shutdown(); }

bool GameApplication::Initialize() {
  if (!PrepareWorkingDirectory()) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to prepare the game data directory");
    return false;
  }

  DebugSetup();
  debug_initialized_ = true;

  auto &config = context_.config;
  config.Load();
  config_loaded_ = true;
  ApplyPadBindings(config.input);
  (void)Mid_SetFlags(config.audio.fix_sysex_bugs ? MID_FLAGS::FIX_SYSEX_BUGS
                                                 : MID_FLAGS::NONE);
  Mid_SetVolume(config.audio.bgm_volume);
  Snd_SetVolumes(config.audio.bgm_volume, config.audio.se_volume);

  if (!context_.display.Initialize()) {
    SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO,
                    "Failed to initialize the graphics backend");
    return false;
  }
  display_initialized_ = true;
  Grp_ScreenshotSetPrefix("screenshots/");

  if (config.audio.bgm_enabled && !BGM_Init(config.audio.soundfont)) {
    SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                "No background music backend is available");
  }
  BGM_SetGainApply(config.audio.bgm_vol_norm);

  const auto data_errors = context_.data.Load(PathForData());
  if (!data_errors.empty()) {
    const auto message = data::FormatLoadErrors(data_errors);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Invalid game data",
                             message.c_str(), nullptr);
    return false;
  }

  if (config.audio.se_enabled && !context_.sound_effects.Load()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Failed to load sound effects");
  }
  (void)context_.music.SetPack(config.audio.bgm_pack);

  flow_ = std::make_unique<gameflow::GameFlow>(context_);
  if (!flow_->Start()) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to start the game flow");
    return false;
  }
  running_ = true;
  return true;
}

int GameApplication::Run() {
  return WndBackend_Run([this] { return Tick(); });
}

bool GameApplication::Tick() {
  if ((SystemKey_Data & SYSKEY_GRP_FULLSCREEN) != 0) {
    (void)context_.display.ToggleFullscreen();
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_UP) != 0) {
    (void)context_.display.CycleScale(+1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_DOWN) != 0) {
    (void)context_.display.CycleScale(-1, false);
  }
  if ((SystemKey_Data & SYSKEY_GRP_SCALE_MODE) != 0) {
    (void)context_.display.ToggleScalingMode();
  }
  if ((SystemKey_Data & SYSKEY_GRP_TURBO) != 0) {
    context_.display.ToggleTurbo();
  }
  if ((SystemKey_Data & SYSKEY_GRP_API) != 0) {
    (void)context_.display.CycleApi();
  }

  SystemKey_Data &=
      ~(SYSKEY_GRP_FULLSCREEN | SYSKEY_GRP_SCALE_UP | SYSKEY_GRP_SCALE_DOWN |
        SYSKEY_GRP_SCALE_MODE | SYSKEY_GRP_TURBO | SYSKEY_GRP_API);
  return flow_->Tick(Key_Data, SystemKey_Data);
}

void GameApplication::SaveConfig() {
  if (const auto topleft = WndBackend_Topleft()) {
    context_.config.graphics.window_left = topleft->first;
    context_.config.graphics.window_top = topleft->second;
  }
  context_.config.Save();
}

void GameApplication::Shutdown() {
  flow_.reset();
  if (running_ && config_loaded_) {
    SaveConfig();
  }
  running_ = false;

  BGM_Cleanup();
  Snd_Cleanup();
  Key_End();
  if (display_initialized_) {
    TextBackend_Cleanup();
    GrpBackend_Cleanup();
    display_initialized_ = false;
  }
  if (debug_initialized_) {
    DebugCleanup();
    debug_initialized_ = false;
  }
}
