/// Replay browser, stage selection, and replay naming UI scene.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "replay_scene.h"

#include "audio/constants.h"
#include "audio/snd.h"
#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
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

bool ReplayScene::EnterBrowser() {
  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
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
  previous_input_ = Key_Data;
  ResetRows();
  mode_ = Mode::Browser;
  GameFlow.game_main = [](bool &quit) {
    GameFlow.ctx.replay_scene.Update(quit);
  };
  GameFlow.current_state = GameState::ReplayBrowser;
  return true;
}

void ReplayScene::BeginSave(bool extra_stage,
                            std::function<void(bool)> on_complete) {
  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
    record_system_.CancelRecording();
    on_complete(false);
    return;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  save_extra_stage_ = extra_stage;
  save_complete_ = std::move(on_complete);
  name_entry_.Begin(true);
  mode_ = Mode::NameEntry;
  GameFlow.game_main = [](bool &quit) {
    GameFlow.ctx.replay_scene.Update(quit);
  };
  GameFlow.current_state = GameState::ReplayNameEntry;
}

void ReplayScene::Update(bool & /*quit*/) {
  switch (mode_) {
  case Mode::Browser:
    UpdateBrowser();
    break;
  case Mode::StageSelect:
    UpdateStageSelect();
    break;
  case Mode::NameEntry:
    UpdateNameEntry();
    break;
  }
}

void ReplayScene::UpdateBrowser() {
  if (Key_Data == 0) {
    previous_input_ = 0;
  } else if (previous_input_ == 0) {
    previous_input_ = Key_Data;
    if (Key_Data == KEY_ESC || Key_Data == KEY_BOMB) {
      Snd_SEPlay(SfxId::Cancel);
      (void)GameExit(false);
      return;
    }
    if (!replays_.empty()) {
      const auto page_count = (replays_.size() + kPageSize - 1) / kPageSize;
      const auto page = selected_ / kPageSize;
      const auto row = selected_ % kPageSize;
      if (Key_Data == KEY_UP) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + page_size - 1) % page_size;
        Snd_SEPlay(SfxId::Select);
      } else if (Key_Data == KEY_DOWN) {
        const auto page_start = page * kPageSize;
        const auto page_size =
            std::min(kPageSize, replays_.size() - page_start);
        selected_ = page_start + (row + 1) % page_size;
        Snd_SEPlay(SfxId::Select);
      } else if (Key_Data == KEY_LEFT || Key_Data == KEY_RIGHT) {
        if (page_count > 1 && !rows_.back().moving) {
          const auto direction = Key_Data == KEY_LEFT ? page_count - 1 : 1;
          const auto next_page = (page + direction) % page_count;
          selected_ =
              std::min(next_page * kPageSize + row, replays_.size() - 1);
          ResetRows();
          Snd_SEPlay(SfxId::Select);
        }
      } else if (Key_Data == KEY_RETURN || Key_Data == KEY_TAMA) {
        Snd_SEPlay(SfxId::Select);
        OpenStageSelect();
      }
    }
  }

  if (GameFlow.IsDraw()) {
    DrawBrowser();
    Grp_Flip();
  }
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
  stage_menu_.Open({240, 150}, std::to_underlying(replay.stages.front()));
  pending_stage_.reset();
  mode_ = Mode::StageSelect;
}

void ReplayScene::UpdateStageSelect() {
  stage_menu_.Tick(Key_Data);
  if (pending_stage_) {
    const auto path = replays_[selected_].path;
    const auto stage = *pending_stage_;
    pending_stage_.reset();
    if (!GameReplayInit(path.c_str(), stage)) {
      (void)EnterBrowser();
    }
    return;
  }
  if (!stage_menu_.Active()) {
    mode_ = Mode::Browser;
    previous_input_ = Key_Data;
  }

  if (GameFlow.IsDraw()) {
    DrawBrowser();
    stage_menu_.Draw();
    Grp_Flip();
  }
}

void ReplayScene::UpdateNameEntry() {
  const auto result = name_entry_.Update(Key_Data);
  if (result != NameEntryResult::Editing) {
    const bool saved =
        result == NameEntryResult::Confirmed &&
        record_system_.SaveReplay(name_entry_.Name(), save_extra_stage_);
    if (result == NameEntryResult::Cancelled) {
      record_system_.CancelRecording();
    }
    auto completion = std::move(save_complete_);
    completion(saved);
    return;
  }

  if (GameFlow.IsDraw()) {
    DrawNameEntry();
    Grp_Flip();
  }
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
  GrpPut16c2(x + 88, y + 4, std::string{name_entry_.Name()}.c_str());
  name_entry_.Draw(x + 88, y + 4);
}
