/// Top-level application lifetime and composition root.

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <SDL3/SDL_messagebox.h>

#include "game_application.h"

#include "data/game_data.h"
#include "gameflow/game_flow.h"
#include "gfx/graphics.h"
#include "gfx/text/text_renderer.h"
#include "gfx/window/window.h"
#include "i18n/localization.h"
#include "settings/config.h"
#include "sys/input.h"
#include "sys/log.h"
#include "sys/path.h"

namespace {

bool PrepareWorkingDirectory() {
  const auto data_path = PathForData();
  std::error_code error;
  std::filesystem::current_path(data_path, error);
  return !static_cast<bool>(error);
}

void ApplyPadBindings(InputSystem &input, const InputConfig &config) {
  const std::array bindings = {
      InputPadBinding{config.pad_tama, KeyTama},
      InputPadBinding{config.pad_bomb, KeyBomb},
      InputPadBinding{config.pad_shift, KeyShift},
      InputPadBinding{config.pad_cancel, KeyEscape},
  };
  input.SetPadBindings(bindings);
}

} // namespace

GameApplication::GameApplication() = default;

GameApplication::~GameApplication() { Shutdown(); }

bool GameApplication::Initialize() {
  if (!InitializeConfig()) {
    return false;
  }
  if (!InitializeLocalization()) {
    return false;
  }
  if (!InitializeGraphics()) {
    return false;
  }
  if (!InitializeAudio()) {
    return false;
  }
  if (!LoadGameData()) {
    return false;
  }

  flow_ = std::make_unique<gameflow::GameFlow>(context_);
  if (!flow_->Start()) {
    logging::Critical(logging::Channel::GameFlow,
                      "Failed to start the game flow");
    return false;
  }
  running_ = true;
  logging::Info(logging::Channel::Application, "Application initialized");
  return true;
}

bool GameApplication::InitializeConfig() {
  if (!PrepareWorkingDirectory()) {
    logging::Critical(logging::Channel::Application,
                      "Failed to prepare the game data directory");
    return false;
  }

  config_ = LoadConfig();
  config_loaded_ = true;
  ApplyPadBindings(input_, config_.input);
  return true;
}

bool GameApplication::InitializeLocalization() {
  if (!context_.localization.Initialize(config_.ui.language)) {
    logging::Critical(logging::Channel::I18n,
                      "Failed to initialize embedded message catalogs");
    return false;
  }
  config_.ui.language = context_.localization.Language();
  return true;
}

bool GameApplication::InitializeGraphics() {
  if (!context_.display.Initialize(config_.graphics)) {
    logging::Critical(logging::Channel::Graphics,
                      "Failed to initialize the graphics backend");
    return false;
  }
  display_initialized_ = true;
  if (!TextInitialize(context_.localization.Language())) {
    logging::Critical(logging::Channel::Graphics,
                      "Failed to initialize the text rendering backend");
    return false;
  }
  GraphicsScreenshotSetPrefix("screenshots/");
  return true;
}

bool GameApplication::InitializeAudio() {
  const auto audio_result =
      context_.audio.Initialize(PathForData(), config_.audio.soundfont);
  context_.audio.SetMidiFixSysExBugs(config_.audio.fix_sysex_bugs);
  context_.audio.SetVolumes(config_.audio.bgm_volume, config_.audio.se_volume);
  if (!audio_result.success) {
    logging::Warning(logging::Channel::Audio,
                     "No background music backend is available");
  }
  return true;
}

bool GameApplication::LoadGameData() {
  const auto data_errors = context_.data.Load(PathForData());
  if (!data_errors.empty()) {
    for (const auto &error : data_errors) {
      const auto *const source = error.source == data::DataSourceKind::Directory
                                     ? "directory"
                                     : "archive";
      const auto *const reason =
          error.kind == data::LoadErrorKind::Missing ? "missing" : "invalid";
      logging::Error(logging::Channel::Data,
                     "Failed to load game data: source={} reason={}", source,
                     reason);
    }
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

  // Sound effects and music packs come from the game data, so they load only
  // after the data has been validated.
  if (config_.audio.se_enabled && !context_.sound_effects.Load()) {
    logging::Warning(logging::Channel::Audio, "Failed to load sound effects");
  }
  context_.music.SetMidiVariant(config_.audio.midi_variant);
  (void)context_.music.SetPack(config_.audio.bgm_pack);

  context_.ui.ConfigureMain(config_, {.display = context_.display,
                                      .input = input_,
                                      .audio = context_.audio,
                                      .sound_effects = context_.sound_effects,
                                      .music = context_.music,
                                      .localization = context_.localization});
  return true;
}

int GameApplication::Run() {
  return WindowRun([this] { input_snapshot_ = input_.Poll(); },
                   [this] { return Tick(); });
}

bool GameApplication::Tick() {
  GraphicsRequestScreenshot((input_snapshot_.system & SystemKeySnapshot) != 0);
  return flow_->Tick(input_snapshot_.game, input_snapshot_.system);
}

void GameApplication::PersistConfig() {
  if (!SaveConfig(config_)) {
    logging::Error(logging::Channel::Application,
                   "Failed to persist application configuration");
  }
}

void GameApplication::Shutdown() {
  flow_.reset();
  if (running_ && config_loaded_) {
    PersistConfig();
  }
  running_ = false;

  context_.audio.Shutdown();
  if (display_initialized_) {
    TextCleanup();
    GraphicsCleanup();
    display_initialized_ = false;
  }
}
