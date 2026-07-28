/// Score leaderboard, detail display, and name registration UI scene.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "record/record_system.h"
#include "ui/name_entry.h"

class MusicPlayer;
class UIManager;

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
             MusicPlayer &music, UIManager &ui)
      : record_system_(records), graphics_(graphics), music_(music), ui_(ui) {}

  [[nodiscard]] bool ShowLeaderboard(GameLevel initial_difficulty,
                                     INPUT_BITS initial_input);
  [[nodiscard]] ScoreRegistrationStart
  StartNameRegistration(ScoreRecord record, INPUT_BITS initial_input,
                        bool change_music = false);
  [[nodiscard]] ScoreSceneResult UpdateLeaderboard(INPUT_BITS input,
                                                   bool should_draw);
  [[nodiscard]] ScoreSceneResult UpdateNameRegistration(INPUT_BITS input,
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
  uint8_t current_difficulty_ = 0;
  bool input_locked_ = false;
  bool detail_open_ = false;
  NameEntry name_entry_;
  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  UIManager &ui_;
};
