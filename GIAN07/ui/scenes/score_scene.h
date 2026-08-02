/// Score leaderboard, detail display, and name registration UI scene.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "gfx/text.h"
#include "record/record_system.h"
#include "ui/name_entry.h"

class MusicPlayer;
class UiManager;

namespace audio {
class AudioSystem;
}

namespace i18n {
class Localization;
}

namespace data {
class GraphicsLoader;
}

enum class ScoreSceneResult : uint8_t {
  Running,
  ExitRequested,
  RegistrationComplete,
};

enum class ScoreRegistrationStart : uint8_t { Active, Complete };

class ScoreScene {
public:
  ScoreScene(RecordSystem &records, data::GraphicsLoader &graphics,
             MusicPlayer &music, UiManager &ui,
             i18n::Localization &localization, audio::AudioSystem &audio)
      : record_system_(records), graphics_(graphics), music_(music), ui_(ui),
        localization_(localization), audio_(audio), name_entry_(audio) {}

  [[nodiscard]] bool ShowLeaderboard(GameLevel initial_difficulty,
                                     InputBits initial_input);
  [[nodiscard]] ScoreRegistrationStart
  StartNameRegistration(ScoreRecord record, InputBits initial_input,
                        bool change_music = false);
  [[nodiscard]] ScoreSceneResult UpdateLeaderboard(InputBits input,
                                                   bool should_draw);
  [[nodiscard]] ScoreSceneResult UpdateNameRegistration(InputBits input,
                                                        bool should_draw);

private:
  static constexpr std::size_t kRowCount = 5;

  struct DisplayRow {
    int x = 0;
    int y = 0;
    bool moving = false;
  };

  void LoadLeaderboard(GameLevel difficulty);
  void ResetRows();
  void DrawLeaderboard(bool show_selection);
  void DrawDetail() const;

  RecordSystem &record_system_;
  std::vector<ScoreRecord> scores_;
  std::array<DisplayRow, kRowCount> rows_{};
  std::optional<ScoreRecord> current_record_;
  std::size_t selected_ = 0;
  std::size_t pending_rank_ = 0;
  int current_difficulty_ = 0;
  bool input_locked_ = false;
  bool detail_open_ = false;
  bool save_failed_ = false;
  TextRenderRectId ui_text_ = 0;
  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  UiManager &ui_;
  i18n::Localization &localization_;
  audio::AudioSystem &audio_;
  NameEntry name_entry_;
};
