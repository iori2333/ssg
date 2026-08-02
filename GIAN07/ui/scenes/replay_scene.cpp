/// Replay browser, stage selection, and replay naming UI scene.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "replay_scene.h"

#include "audio/audio_system.h"
#include "audio/sfx.h"
#include "data/graphics_loader.h"
#include "gameplay/game_rules.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/geometry.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text.h"
#include "i18n/localization.h"
#include "platform/text_backend.h"
#include "player/loadout/player_loadout.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "sys/log.h"
#include "ui/menu/menu_controller.h"
#include "ui/menu/menu_tree.h"
#include "ui/name_entry.h"
#include "ui/ui_manager.h"
#include "util/time_api.h"

namespace {
std::string_view Text(const i18n::Localization &localization,
                      std::string_view key) {
  return localization.Text(i18n::TextIdFromKey(key));
}

std::string_view StageName(const i18n::Localization &localization,
                           StageId stage) {
  constexpr std::array keys = {"ui.value.stage1", "ui.value.stage2",
                               "ui.value.stage3", "ui.value.stage4",
                               "ui.value.stage5", "ui.value.stage6",
                               "ui.value.extra"};
  const auto index = std::to_underlying(stage);
  return index < keys.size() ? Text(localization, keys[index])
                             : std::string_view{};
}

std::string_view PlayerName(const i18n::Localization &localization,
                            PlayerType player) {
  constexpr std::array keys = {"ui.value.wide", "ui.value.homing",
                               "ui.value.laser"};
  const auto index = std::to_underlying(player);
  return index < keys.size() ? Text(localization, keys[index])
                             : std::string_view{};
}

std::string StageList(const ReplayRecord &replay) {
  std::string result;
  for (const auto stage : replay.stages) {
    if (!result.empty()) {
      result += ' ';
    }
    if (stage == StageId::Extra) {
      result += "EX";
    } else {
      result += std::to_string(std::to_underlying(stage) + 1);
    }
  }
  return result;
}

uint8_t DetailGradient(PixelCoord y) {
  if (y <= 3) {
    return 254;
  }
  return y <= 6 ? 220 : 180;
}

void RenderUiText(WindowPoint position, TextRenderRectId rect,
                  std::string_view text, bool centered = false) {
  TextRenderer().Render(
      position, rect, text, [text, centered](TextRenderSession &s) {
        s.SetFont(FontId::Normal);
        const auto x = centered ? TextLayoutXCenter(s, text) : 0;
        s.Put({.x = x + 1, .y = 1}, text, Rgb{.r = 96, .g = 96, .b = 96});
        s.Put({.x = x, .y = 0}, text, Rgb{.r = 255, .g = 255, .b = 255});
      });
}
} // namespace

bool ReplayScene::EnterBrowser(InputBits initial_input) {
  ui_.ForceCloseMessageWindow();
  GraphicsBackendClear();
  GraphicsFlip();
  if (!graphics_.LoadNameRegistration()) {
    logging::Error(logging::Channel::Ui,
                   "Failed to load Replay screen graphics");
    return false;
  }

  GraphicsBackendSetClip(kGameResolutionRect);
  TextRenderer().Clear();
  for (auto &text : stage_text_) {
    text = TextRenderer().Register({.w = 80, .h = 10});
  }
  for (auto &text : player_text_) {
    text = TextRenderer().Register({.w = 80, .h = 10});
  }
  ui_text_ = TextRenderer().Register({.w = 480, .h = 24});
  replays_ = RecordSystem::ListReplays();
  selected_ = 0;
  previous_input_ = initial_input;
  ResetRows();
  mode_ = Mode::Browser;
  return true;
}

bool ReplayScene::BeginSave(bool extra_stage, InputBits initial_input) {
  ui_.ForceCloseMessageWindow();
  GraphicsBackendClear();
  GraphicsFlip();
  if (!graphics_.LoadNameRegistration()) {
    record_system_.CancelRecording();
    return false;
  }

  GraphicsBackendSetClip(kGameResolutionRect);
  TextRenderer().Clear();
  ui_text_ = TextRenderer().Register({.w = 480, .h = 24});
  save_extra_stage_ = extra_stage;
  save_failed_ = false;
  name_entry_.Begin(true, initial_input);
  mode_ = Mode::NameEntry;
  return true;
}

ReplaySceneResult ReplayScene::Update(InputBits input, bool should_draw) {
  switch (mode_) {
  case Mode::Browser:
    return UpdateBrowser(input, should_draw);
  case Mode::StageSelect:
    return UpdateStageSelect(input, should_draw);
  case Mode::NameEntry:
    return UpdateNameEntry(input, should_draw);
  }
  std::unreachable();
}

ReplaySceneResult ReplayScene::UpdateBrowser(InputBits input,
                                             bool should_draw) {
  if (input == 0) {
    previous_input_ = 0;
  } else if (previous_input_ == 0) {
    previous_input_ = input;
    if (input == KeyEscape || input == KeyBomb) {
      audio_.PlaySfx(SfxId::Cancel);
      return {.type = ReplaySceneResult::Type::ExitRequested};
    }
    if (!replays_.empty()) {
      const auto page_count = (replays_.size() + kPageSize - 1) / kPageSize;
      const auto page = selected_ / kPageSize;
      const auto row = selected_ % kPageSize;
      if (input == KeyUp) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + page_size - 1) % page_size;
        audio_.PlaySfx(SfxId::Select);
      } else if (input == KeyDown) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + 1) % page_size;
        audio_.PlaySfx(SfxId::Select);
      } else if (input == KeyLeft || input == KeyRight) {
        if (page_count > 1 && !rows_.back().moving) {
          const auto direction = input == KeyLeft ? page_count - 1 : 1;
          const auto next_page = (page + direction) % page_count;
          selected_ =
              std::min(next_page * kPageSize + row, replays_.size() - 1);
          ResetRows();
          audio_.PlaySfx(SfxId::Select);
        }
      } else if (input == KeyReturn || input == KeyTama) {
        audio_.PlaySfx(SfxId::Select);
        OpenStageSelect();
      }
    }
  }

  if (should_draw) {
    DrawBrowser();
    GraphicsFlip();
  }
  return {};
}

void ReplayScene::OpenStageSelect() {
  const auto &replay = replays_[selected_];
  std::vector<std::unique_ptr<menu::IMenuNode>> items;
  items.reserve(kStageNames.size());
  for (int id = 0; id <= std::to_underlying(StageId::Extra); id++) {
    const auto stage = static_cast<StageId>(id);
    auto item = std::make_unique<menu::ActionNode>(
        menu::MenuText(
            [this, stage] { return StageName(localization_, stage); }),
        "", [this, stage](menu::MenuController &) {
          pending_stage_ = stage;
          return false;
        });
    item->SetEnabled(std::ranges::contains(replay.stages, stage));
    items.push_back(std::move(item));
  }
  stage_menu_root_ = std::make_unique<menu::EntryNode>(
      menu::MenuText(
          [this] { return Text(localization_, "ui.replay.select_stage"); }),
      "", std::move(items));
  stage_menu_.SetExitText(
      menu::MenuText([this] { return Text(localization_, "ui.common.back"); }),
      menu::MenuText(
          [this] { return Text(localization_, "ui.common.back_help"); }));
  stage_menu_.Init(160);
  stage_menu_.Navigate(*stage_menu_root_,
                       std::to_underlying(replay.stages.front()));
  stage_menu_.Open({240, 150}, std::to_underlying(replay.stages.front()),
                   previous_input_);
  pending_stage_.reset();
  mode_ = Mode::StageSelect;
}

ReplaySceneResult ReplayScene::UpdateStageSelect(InputBits input,
                                                 bool should_draw) {
  stage_menu_.Tick(input);
  if (pending_stage_) {
    const auto path = replays_[selected_].path;
    const auto stage = *pending_stage_;
    pending_stage_.reset();
    return {
        .type = ReplaySceneResult::Type::PlaybackRequested,
        .replay_path = path,
        .stage = stage,
    };
  }
  if (!stage_menu_.Active()) {
    mode_ = Mode::Browser;
    previous_input_ = input;
  }

  if (should_draw) {
    DrawBrowser();
    stage_menu_.Draw();
    GraphicsFlip();
  }
  return {};
}

ReplaySceneResult ReplayScene::UpdateNameEntry(InputBits input,
                                               bool should_draw) {
  const auto result = name_entry_.Update(input);
  if (result == NameEntryResult::Confirmed) {
    const auto save_result =
        record_system_.SaveReplay(name_entry_.Name(), save_extra_stage_);
    if (save_result == RecordSaveResult::Saved) {
      return {.type = ReplaySceneResult::Type::SaveComplete, .saved = true};
    }
    save_failed_ = true;
    audio_.PlaySfx(SfxId::Cancel);
  } else if (result == NameEntryResult::Cancelled) {
    record_system_.CancelRecording();
    return {.type = ReplaySceneResult::Type::SaveComplete, .saved = false};
  }

  if (should_draw) {
    DrawNameEntry();
    GraphicsFlip();
  }
  return {};
}

void ReplayScene::ResetRows() {
  for (std::size_t i = 0; i < rows_.size(); i++) {
    rows_[i] = {
        .x = static_cast<int>((640 + 50 + (i * 24 * 20)) << 6),
        .y = static_cast<int>((100 + (i * 48)) << 6),
        .moving = true,
    };
  }
}

void ReplayScene::DrawBrowser() {
  GraphicsBackendClear();

  const auto page = replays_.empty() ? 0 : selected_ / kPageSize;
  const auto first = page * kPageSize;
  const auto last = std::min(first + kPageSize, replays_.size());
  for (std::size_t row = 0; row < rows_.size(); row++) {
    auto &display = rows_[row];
    const auto target_x = static_cast<int>((50 + (row * 24)) << 6);
    const auto velocity = (display.x - target_x) / 12;
    if (velocity > 2_px) {
      display.x -= velocity;
    } else {
      display.moving = false;
    }

    const int x = display.x >> 6;
    const int y = display.y >> 6;
    GraphicsSurfaceBlit({x, y}, SurfaceId::NameRegistration,
                        PixelLtwh{0, 64 + static_cast<int>(row * 32), 400, 32});
    const auto index = first + row;
    if (index >= last) {
      continue;
    }
    geometry::SetColor({0, 0, 0});
    geometry::DrawBox(x + 88, y + 27, x + 114, y + 32);
    if (index == selected_) {
      geometry::SetAlphaNorm(96);
      geometry::SetColor({4, 0, 0});
      geometry::DrawBoxA(x, y, x + 400, y + 32);
    }
    const auto &replay = replays_[index];
    DrawFont16C2(x + 88, y + 4, replay.name.c_str());

    const auto difficulty = GameLevelName(replay.difficulty);
    const auto difficulty_x =
        x + 384 - static_cast<int>(difficulty.size() * 14);
    DrawFont16(difficulty_x, y + 4, std::string(difficulty).c_str());

    const auto date = std::format(
        "{:%Y/%m/%d %H:%M}", util::LocalTime(util::UtcTime(replay.created_at)));
    constexpr int detail_y = 25;
    DrawScore(x + 88, y + detail_y, date.c_str());

    const auto stages = StageList(replay);
    TextRenderer().Render({x + 224, y + detail_y - 2}, stage_text_[row], stages,
                          [&stages](TextRenderSession &session) {
                            std::array<std::string_view, 1> text = {stages};
                            DrawGrdFont(session, text, FontId::Tiny, false,
                                        DetailGradient);
                          });

    const auto player = PlayerName(localization_, replay.player_type);
    TextRenderer().Render({x + 304, y + detail_y - 2}, player_text_[row],
                          player, [player](TextRenderSession &session) {
                            std::array<std::string_view, 1> text = {player};
                            DrawGrdFont(session, text, FontId::Tiny, false,
                                        DetailGradient);
                          });
  }

  if (replays_.empty()) {
    const auto message = Text(localization_, "ui.replay.no_data");
    RenderUiText({80, 448}, ui_text_, message, true);
    return;
  }

  const auto page_count = (replays_.size() + kPageSize - 1) / kPageSize;
  const auto page_label = std::format(
      "{}  {}/{}", Text(localization_, "ui.replay.page"), page + 1, page_count);
  RenderUiText({80, 448}, ui_text_, page_label, true);
}

void ReplayScene::DrawNameEntry() const {
  GraphicsBackendClear();
  const int x = 120;
  const int y = 176;
  geometry::SetColor({2, 0, 0});
  geometry::DrawBox(x, y, x + 400, y + 32);
  GraphicsSurfaceBlit({x, y}, SurfaceId::NameRegistration, {0, 64, 400, 96});
  const auto title = Text(localization_, "ui.replay.name");
  RenderUiText({80, 118}, ui_text_, title, true);
  if (save_failed_) {
    const auto failed = Text(localization_, "ui.replay.save_failed");
    RenderUiText({80, 150}, ui_text_, failed, true);
  }
  DrawFont16C2(x + 88, y + 4, std::string{name_entry_.Name()}.c_str());
  name_entry_.Draw(x + 88, y + 4);
}
