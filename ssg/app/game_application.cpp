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
  if (!PrepareWorkingDirectory()) {
    logging::Critical(logging::Channel::Application,
                      "Failed to prepare the game data directory");
    return false;
  }

  config_ = LoadConfig();
  config_loaded_ = true;
  const auto &config = config_;
  ApplyPadBindings(input_, config.input);

  if (!context_.localization.Initialize(config.ui.language)) {
    logging::Critical(logging::Channel::I18n,
                      "Failed to initialize embedded message catalogs");
    return false;
  }
  config_.ui.language = context_.localization.Language();

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

  const auto audio_result =
      context_.audio.Initialize(PathForData(), config.audio.soundfont);
  context_.audio.SetMidiFixSysExBugs(config.audio.fix_sysex_bugs);
  context_.audio.SetVolumes(config.audio.bgm_volume, config.audio.se_volume);
  if (!audio_result.success) {
    logging::Warning(logging::Channel::Audio,
                     "No background music backend is available");
  }

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

  if (config.audio.se_enabled && !context_.sound_effects.Load()) {
    logging::Warning(logging::Channel::Audio, "Failed to load sound effects");
  }
  context_.music.SetMidiVariant(config.audio.midi_variant);
  (void)context_.music.SetPack(config.audio.bgm_pack);
  context_.ui.ConfigureMain(config_, {.display = context_.display,
                                      .input = input_,
                                      .audio = context_.audio,
                                      .sound_effects = context_.sound_effects,
                                      .music = context_.music,
                                      .localization = context_.localization});

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

int GameApplication::Run() {
  return WindowRun([this] { input_snapshot_ = input_.Poll(); },
                   [this] { return Tick(); });
}

bool GameApplication::Tick() {
  GraphicsRequestScreenshot((input_snapshot_.system & SystemKeySnapshot) != 0);
  return flow_->Tick(input_snapshot_.game, input_snapshot_.system);
}

void GameApplication::PersistConfig() { SaveConfig(config_); }

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
