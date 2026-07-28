/// Replay browser, stage selection, and replay naming UI scene.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include "gfx/text.h"
#include "record/record_system.h"
#include "ui/menu/menu_controller.h"
#include "ui/menu/menu_tree.h"
#include "ui/name_entry.h"

class ReplayScene {
public:
  explicit ReplayScene(RecordSystem &records) : record_system_(records) {}

  [[nodiscard]] bool EnterBrowser();
  void BeginSave(bool extra_stage, std::function<void(bool)> on_complete);
  void Update(bool &quit);

private:
  static constexpr std::size_t kPageSize = 5;

  enum class Mode : uint8_t { Browser, StageSelect, NameEntry };

  void UpdateBrowser();
  void UpdateStageSelect();
  void UpdateNameEntry();
  void OpenStageSelect();
  void ResetRows();
  void DrawBrowser();
  void DrawNameEntry() const;

  struct DisplayRow {
    int x = 0;
    int y = 0;
    bool moving = false;
  };

  RecordSystem &record_system_;
  Mode mode_ = Mode::Browser;
  std::vector<ReplayRecord> replays_;
  std::size_t selected_ = 0;
  INPUT_BITS previous_input_ = 0;
  std::array<TEXTRENDER_RECT_ID, kPageSize> stage_text_{};
  std::array<TEXTRENDER_RECT_ID, kPageSize> player_text_{};
  std::array<DisplayRow, kPageSize> rows_{};

  std::unique_ptr<menu::EntryNode> stage_menu_root_;
  menu::MenuController stage_menu_;
  std::optional<StageId> pending_stage_;

  NameEntry name_entry_;
  bool save_extra_stage_ = false;
  std::function<void(bool)> save_complete_;
};
