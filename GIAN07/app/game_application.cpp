/// Top-level application lifetime and composition root.

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

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
#include "i18n/localization.h"
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

  config_ = LoadConfig();
  config_loaded_ = true;
  const auto &config = config_;
  ApplyPadBindings(config.input);
  (void)Mid_SetFlags(config.audio.fix_sysex_bugs ? MID_FLAGS::FIX_SYSEX_BUGS
                                                 : MID_FLAGS::NONE);
  Mid_SetVolume(config.audio.bgm_volume);
  Snd_SetVolumes(config.audio.bgm_volume, config.audio.se_volume);

  if (!context_.localization.Initialize(config.ui.language)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to initialize embedded message catalogs");
    return false;
  }
  config_.ui.language = context_.localization.Language();

  if (!context_.display.Initialize(config_.graphics)) {
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
    const auto text = [this](std::string_view key) {
      return context_.localization.Text(i18n::TextIdFromKey(key));
    };
    const auto message =
        data::FormatLoadErrors(data_errors, text("ui.error.file_not_found"),
                               text("ui.error.invalid_archive"));
    const auto title = text("ui.error.invalid_game_data");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, std::string(title).c_str(),
                             message.c_str(), nullptr);
    return false;
  }

  if (config.audio.se_enabled && !context_.sound_effects.Load()) {
    SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Failed to load sound effects");
  }
  context_.music.SetMidiVariant(config.audio.midi_variant);
  (void)context_.music.SetPack(config.audio.bgm_pack);
  context_.ui.ConfigureMain(config_, {.display = context_.display,
                                      .sound_effects = context_.sound_effects,
                                      .music = context_.music,
                                      .localization = context_.localization});

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

bool GameApplication::Tick() { return flow_->Tick(Key_Data, SystemKey_Data); }

void GameApplication::PersistConfig() { SaveConfig(config_); }

void GameApplication::Shutdown() {
  flow_.reset();
  if (running_ && config_loaded_) {
    PersistConfig();
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
