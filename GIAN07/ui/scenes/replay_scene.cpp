/// Replay browser, stage selection, and replay naming UI scene.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "replay_scene.h"

#include "audio/constants.h"
#include "audio/snd.h"
#include "data/graphics_loader.h"
#include "gameplay/game_rules.h"
#include "gfx/constants.h"
#include "gfx/coords.h"
#include "gfx/font_uty.h"
#include "gfx/graphics.h"
#include "gfx/graphics_backend.h"
#include "gfx/text.h"
#include "platform/text_backend.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "ui/menu/menu_controller.h"
#include "ui/menu/menu_tree.h"
#include "ui/name_entry.h"
#include "ui/ui_manager.h"
#include "util/debug.h"

namespace {
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

uint8_t DetailGradient(PIXEL_COORD y) {
  if (y <= 3) {
    return 254;
  }
  return y <= 6 ? 220 : 180;
}
} // namespace

bool ReplayScene::EnterBrowser(INPUT_BITS initial_input) {
  ui_.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!graphics_.LoadNameRegistration()) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  TextObj.Clear();
  for (auto &text : stage_text_) {
    text = TextObj.Register({.w = 80, .h = 10});
  }
  for (auto &text : player_text_) {
    text = TextObj.Register({.w = 80, .h = 10});
  }
  replays_ = record_system_.ListReplays();
  selected_ = 0;
  previous_input_ = initial_input;
  ResetRows();
  mode_ = Mode::Browser;
  return true;
}

bool ReplayScene::BeginSave(bool extra_stage, INPUT_BITS initial_input) {
  ui_.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!graphics_.LoadNameRegistration()) {
    record_system_.CancelRecording();
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  save_extra_stage_ = extra_stage;
  save_failed_ = false;
  name_entry_.Begin(true, initial_input);
  mode_ = Mode::NameEntry;
  return true;
}

ReplaySceneResult ReplayScene::Update(INPUT_BITS input, bool should_draw) {
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

ReplaySceneResult ReplayScene::UpdateBrowser(INPUT_BITS input,
                                             bool should_draw) {
  if (input == 0) {
    previous_input_ = 0;
  } else if (previous_input_ == 0) {
    previous_input_ = input;
    if (input == KEY_ESC || input == KEY_BOMB) {
      Snd_SEPlay(SfxId::Cancel);
      return {.type = ReplaySceneResult::Type::ExitRequested};
    }
    if (!replays_.empty()) {
      const auto page_count = (replays_.size() + kPageSize - 1) / kPageSize;
      const auto page = selected_ / kPageSize;
      const auto row = selected_ % kPageSize;
      if (input == KEY_UP) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + page_size - 1) % page_size;
        Snd_SEPlay(SfxId::Select);
      } else if (input == KEY_DOWN) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + 1) % page_size;
        Snd_SEPlay(SfxId::Select);
      } else if (input == KEY_LEFT || input == KEY_RIGHT) {
        if (page_count > 1 && !rows_.back().moving) {
          const auto direction = input == KEY_LEFT ? page_count - 1 : 1;
          const auto next_page = (page + direction) % page_count;
          selected_ =
              std::min(next_page * kPageSize + row, replays_.size() - 1);
          ResetRows();
          Snd_SEPlay(SfxId::Select);
        }
      } else if (input == KEY_RETURN || input == KEY_TAMA) {
        Snd_SEPlay(SfxId::Select);
        OpenStageSelect();
      }
    }
  }

  if (should_draw) {
    DrawBrowser();
    Grp_Flip();
  }
  return {};
}

void ReplayScene::OpenStageSelect() {
  const auto &replay = replays_[selected_];
  std::vector<std::unique_ptr<menu::IMenuNode>> items;
  items.reserve(kStageNames.size());
  for (uint8_t id = 0; id <= std::to_underlying(StageId::Extra); id++) {
    const auto stage = static_cast<StageId>(id);
    auto item = std::make_unique<menu::ActionNode>(
        StageName(stage), "", [this, stage](menu::MenuController &) {
          pending_stage_ = stage;
          return false;
        });
    item->SetEnabled(std::ranges::contains(replay.stages, stage));
    items.push_back(std::move(item));
  }
  stage_menu_root_ =
      std::make_unique<menu::EntryNode>("Select Stage", "", std::move(items));
  stage_menu_.Init(160);
  stage_menu_.Navigate(*stage_menu_root_,
                       std::to_underlying(replay.stages.front()));
  stage_menu_.Open({240, 150}, std::to_underlying(replay.stages.front()),
                   previous_input_);
  pending_stage_.reset();
  mode_ = Mode::StageSelect;
}

ReplaySceneResult ReplayScene::UpdateStageSelect(INPUT_BITS input,
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
    Grp_Flip();
  }
  return {};
}

ReplaySceneResult ReplayScene::UpdateNameEntry(INPUT_BITS input,
                                               bool should_draw) {
  const auto result = name_entry_.Update(input);
  if (result == NameEntryResult::Confirmed) {
    const auto save_result =
        record_system_.SaveReplay(name_entry_.Name(), save_extra_stage_);
    if (save_result == RecordSaveResult::Saved) {
      return {.type = ReplaySceneResult::Type::SaveComplete, .saved = true};
    }
    save_failed_ = true;
    Snd_SEPlay(SfxId::Cancel);
  } else if (result == NameEntryResult::Cancelled) {
    record_system_.CancelRecording();
    return {.type = ReplaySceneResult::Type::SaveComplete, .saved = false};
  }

  if (should_draw) {
    DrawNameEntry();
    Grp_Flip();
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
  GrpBackend_Clear();

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
    GrpSurface_Blit({x, y}, SURFACE_ID::NAMEREG,
                    PIXEL_LTWH{0, 64 + static_cast<int>(row * 32), 400, 32});
    const auto index = first + row;
    if (index >= last) {
      continue;
    }
    GrpGeom->Lock();
    GrpGeom->SetColor({0, 0, 0});
    GrpGeom->DrawBox(x + 88, y + 27, x + 114, y + 32);
    GrpGeom->Unlock();
    if (index == selected_) {
      GrpGeom->Lock();
      GrpGeom->SetAlphaNorm(96);
      GrpGeom->SetColor({4, 0, 0});
      GrpGeom->DrawBoxA(x, y, x + 400, y + 32);
      GrpGeom->Unlock();
    }
    const auto &replay = replays_[index];
    GrpPut16c2(x + 88, y + 4, replay.name.c_str());

    const auto difficulty = GameLevelName(replay.difficulty);
    const auto difficulty_x =
        x + 384 - static_cast<int>(difficulty.size() * 14);
    GrpPut16(difficulty_x, y + 4, std::string{difficulty}.c_str());

    const auto date = std::format("{:%Y/%m/%d %H:%M}",
                                  std::chrono::system_clock::time_point{
                                      std::chrono::seconds{replay.created_at}});
    constexpr int detail_y = 25;
    GrpPutScore(x + 88, y + detail_y, date.c_str());

    const auto stages = StageList(replay);
    TextObj.Render({x + 224, y + detail_y - 2}, stage_text_[row], stages,
                   [&stages](TEXTRENDER_SESSION &session) {
                     std::array<std::string_view, 1> text = {stages};
                     DrawGrdFont(session, text, FONT_ID::TINY, false,
                                 DetailGradient);
                   });

    const auto player = PlayerTypeName(replay.player_type);
    TextObj.Render({x + 304, y + detail_y - 2}, player_text_[row], player,
                   [player](TEXTRENDER_SESSION &session) {
                     std::array<std::string_view, 1> text = {player};
                     DrawGrdFont(session, text, FONT_ID::TINY, false,
                                 DetailGradient);
                   });
  }

  if (replays_.empty()) {
    GrpPut16(240, 450, "No Replay Data");
    return;
  }

  const auto page_count = (replays_.size() + kPageSize - 1) / kPageSize;
  const auto page_label = std::format("Replay  {}/{}", page + 1, page_count);
  GrpPut16(272, 450, page_label.c_str());
}

void ReplayScene::DrawNameEntry() const {
  GrpBackend_Clear();
  const int x = 120;
  const int y = 176;
  GrpGeom->Lock();
  GrpGeom->SetColor({2, 0, 0});
  GrpGeom->DrawBox(x, y, x + 400, y + 32);
  GrpGeom->Unlock();
  GrpSurface_Blit({x, y}, SURFACE_ID::NAMEREG, {0, 64, 400, 96});
  GrpPut16(256, 120, "Replay Name");
  if (save_failed_) {
    GrpPut16(216, 152, "Replay save failed");
  }
  GrpPut16c2(x + 88, y + 4, std::string{name_entry_.Name()}.c_str());
  name_entry_.Draw(x + 88, y + 4);
}
