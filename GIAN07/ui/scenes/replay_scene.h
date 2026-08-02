/// Replay browser, stage selection, and replay naming UI scene.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "gfx/text.h"
#include "record/record_system.h"
#include "ui/menu/menu_controller.h"
#include "ui/menu/menu_tree.h"
#include "ui/name_entry.h"

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

struct ReplaySceneResult {
  enum class Type : uint8_t {
    Running,
    ExitRequested,
    PlaybackRequested,
    SaveComplete,
  } type = Type::Running;
  std::string replay_path;
  std::optional<StageId> stage;
  bool saved = false;
};

class ReplayScene {
public:
  ReplayScene(RecordSystem &records, data::GraphicsLoader &graphics,
              UiManager &ui, i18n::Localization &localization,
              audio::AudioSystem &audio)
      : record_system_(records), graphics_(graphics), ui_(ui),
        localization_(localization), audio_(audio), stage_menu_(audio),
        name_entry_(audio) {}

  [[nodiscard]] bool EnterBrowser(InputBits initial_input);
  [[nodiscard]] bool BeginSave(bool extra_stage, InputBits initial_input);
  [[nodiscard]] ReplaySceneResult Update(InputBits input, bool should_draw);

private:
  static constexpr std::size_t kPageSize = 5;

  enum class Mode : uint8_t { Browser, StageSelect, NameEntry };

  [[nodiscard]] ReplaySceneResult UpdateBrowser(InputBits input,
                                                bool should_draw);
  [[nodiscard]] ReplaySceneResult UpdateStageSelect(InputBits input,
                                                    bool should_draw);
  [[nodiscard]] ReplaySceneResult UpdateNameEntry(InputBits input,
                                                  bool should_draw);
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
  data::GraphicsLoader &graphics_;
  UiManager &ui_;
  i18n::Localization &localization_;
  audio::AudioSystem &audio_;
  Mode mode_ = Mode::Browser;
  std::vector<ReplayRecord> replays_;
  std::size_t selected_ = 0;
  InputBits previous_input_ = 0;
  std::array<TextRenderRectId, kPageSize> stage_text_{};
  std::array<TextRenderRectId, kPageSize> player_text_{};
  TextRenderRectId ui_text_ = 0;
  std::array<DisplayRow, kPageSize> rows_{};

  std::unique_ptr<menu::EntryNode> stage_menu_root_;
  menu::MenuController stage_menu_;
  std::optional<StageId> pending_stage_;

  NameEntry name_entry_;
  bool save_extra_stage_ = false;
  bool save_failed_ = false;
};
