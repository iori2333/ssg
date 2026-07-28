/// Score leaderboard, detail display, and name registration UI scene.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "record/record_system.h"
#include "ui/name_entry.h"

class ScoreScene {
public:
  explicit ScoreScene(RecordSystem &records) : record_system_(records) {}

  [[nodiscard]] bool ShowLeaderboard(GameLevel initial_difficulty);
  [[nodiscard]] bool
  StartNameRegistration(ScoreRecord record, bool change_music = false,
                        std::function<void()> on_complete = {});
  void UpdateLeaderboard(bool &quit);
  void UpdateNameRegistration(bool &quit);

private:
  static constexpr std::size_t kRowCount = 5;

  struct DisplayRow {
    int x = 0;
    int y = 0;
    bool moving = false;
  };

  void LoadLeaderboard(GameLevel difficulty);
  void FinishRegistration();
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
  std::function<void()> on_registration_complete_;
};
