/// Score leaderboard, detail display, and name registration UI scene.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "score_scene.h"

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
#include "player/loadout/player_loadout.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "ui/name_entry.h"
#include "util/debug.h"

namespace {
constexpr auto kDefaultScoreName = "Vivit!";

std::string RecordDate(int64_t timestamp) {
  return std::format("{:%Y-%m-%d %H%M}", std::chrono::system_clock::time_point{
                                             std::chrono::seconds{timestamp}});
}
} // namespace

void ScoreScene::LoadLeaderboard(GameLevel difficulty) {
  scores_ = record_system_.ListScores(difficulty, kRowCount);
  selected_ = 0;
  detail_open_ = false;
  ResetRows();
}

bool ScoreScene::ShowLeaderboard(GameLevel initial_difficulty) {
  current_difficulty_ = std::to_underlying(initial_difficulty);
  LoadLeaderboard(static_cast<GameLevel>(current_difficulty_));

  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
    DebugOut("IMAGES.PAK が破壊されています");
    return false;
  }

  GrpBackend_SetClip(GRP_RES_RECT);
  input_locked_ = Key_Data != 0U;
  GameFlow.game_main = [](bool &quit) {
    GameFlow.ctx.score.UpdateLeaderboard(quit);
  };
  GameFlow.current_state = GameState::Leaderboard;
  return true;
}

void ScoreScene::UpdateLeaderboard(bool & /*unused*/) {
  if (Key_Data == 0U) {
    input_locked_ = false;
  } else if (!input_locked_) {
    input_locked_ = true;
    if (detail_open_) {
      if (Input_IsOK(Key_Data) || Input_IsCancel(Key_Data)) {
        detail_open_ = false;
        Snd_SEPlay(SfxId::Cancel);
      }
    } else if (Key_Data == KEY_ESC || Key_Data == KEY_BOMB) {
      Snd_SEPlay(SfxId::Cancel);
      (void)GameExit(false);
      return;
    } else if (Key_Data == KEY_UP && !scores_.empty()) {
      selected_ = (selected_ + scores_.size() - 1) % scores_.size();
      Snd_SEPlay(SfxId::Select);
    } else if (Key_Data == KEY_DOWN && !scores_.empty()) {
      selected_ = (selected_ + 1) % scores_.size();
      Snd_SEPlay(SfxId::Select);
    } else if ((Key_Data == KEY_LEFT || Key_Data == KEY_RIGHT) &&
               !rows_.back().moving) {
      const auto level_count = kGameLevelNames.size();
      const auto direction = Key_Data == KEY_LEFT ? level_count - 1 : 1;
      current_difficulty_ =
          static_cast<uint8_t>((current_difficulty_ + direction) % level_count);
      LoadLeaderboard(static_cast<GameLevel>(current_difficulty_));
      Snd_SEPlay(SfxId::Select);
    } else if (Input_IsOK(Key_Data) && !scores_.empty()) {
      detail_open_ = true;
      Snd_SEPlay(SfxId::Select);
    }
  }

  if (GameFlow.IsDraw()) {
    DrawLeaderboard(true);
    if (detail_open_) {
      DrawDetail();
    }
    Grp_Flip();
  }
}

bool ScoreScene::StartNameRegistration(ScoreRecord record, bool change_music,
                                       std::function<void()> on_complete) {
  on_registration_complete_ = std::move(on_complete);
  current_record_.emplace(std::move(record));
  scores_ = record_system_.ListScores(current_record_->difficulty, kRowCount);
  const auto position =
      std::ranges::find_if(scores_, [this](const ScoreRecord &record) {
        return current_record_->score > record.score;
      });
  pending_rank_ = static_cast<std::size_t>(position - scores_.begin());
  if (pending_rank_ >= kRowCount) {
    current_record_->name = kDefaultScoreName;
    (void)record_system_.SaveScore(*current_record_);
    FinishRegistration();
    return true;
  }
  scores_.insert(position, *current_record_);
  if (scores_.size() > kRowCount) {
    scores_.erase(scores_.begin() + static_cast<std::ptrdiff_t>(kRowCount),
                  scores_.end());
  }

  Snd_SEStop(8);
  Snd_SEStopAll();
  GameFlow.ctx.ui.ForceCloseMessageWindow();
  GrpBackend_Clear();
  Grp_Flip();
  if (!GameFlow.ctx.graphics.LoadNameRegistration()) {
    DebugOut("IMAGES.PAK が破壊されています");
    FinishRegistration();
    return false;
  }

  current_difficulty_ = std::to_underlying(current_record_->difficulty);
  ResetRows();
  GrpBackend_SetClip(GRP_RES_RECT);
  name_entry_.Begin(false);
  GameFlow.game_main = [](bool &quit) {
    GameFlow.ctx.score.UpdateNameRegistration(quit);
  };
  GameFlow.current_state = GameState::NameRegistration;
  if (change_music) {
    GameFlow.ctx.music.Play(19);
  }
  return true;
}

void ScoreScene::FinishRegistration() {
  auto completion = std::move(on_registration_complete_);
  if (completion) {
    completion();
  } else {
    (void)GameExit();
  }
}

void ScoreScene::UpdateNameRegistration(bool & /*unused*/) {
  const auto result = name_entry_.Update(Key_Data);
  scores_[pending_rank_].name = name_entry_.Name();
  if (result == NameEntryResult::Confirmed) {
    current_record_->name = name_entry_.Name();
    (void)record_system_.SaveScore(*current_record_);
    FinishRegistration();
    return;
  }

  if (GameFlow.IsDraw()) {
    DrawLeaderboard(false);
    const auto gx = rows_[pending_rank_].x >> 6;
    const auto gy = rows_[pending_rank_].y >> 6;
    name_entry_.Draw(gx + 88, gy + 4);
    Grp_Flip();
  }
}

void ScoreScene::ResetRows() {
  for (std::size_t i = 0; i < rows_.size(); i++) {
    rows_[i] = {
        .x = static_cast<int>((640 + 50 + (i * 24 * 20)) << 6),
        .y = static_cast<int>((100 + (i * 48)) << 6),
        .moving = true,
    };
  }
}

void ScoreScene::DrawLeaderboard(bool show_selection) {
  GrpBackend_Clear();
  for (std::size_t i = 0; i < rows_.size(); i++) {
    auto &row = rows_[i];
    const auto target_x = static_cast<int>((50 + i * 24) << 6);
    const auto velocity = (row.x - target_x) / 12;
    if (velocity > 2_px) {
      row.x -= velocity;
    } else {
      row.moving = false;
    }

    const int x = row.x >> 6;
    const int y = row.y >> 6;
    GrpSurface_Blit({x, y}, SURFACE_ID::NAMEREG,
                    PIXEL_LTWH{0, 64 + static_cast<int>(32 * i), 400, 32});
    if (i >= scores_.size()) {
      continue;
    }
    if (show_selection && i == selected_) {
      GrpGeom->Lock();
      GrpGeom->SetAlphaNorm(96);
      GrpGeom->SetColor({4, 0, 0});
      GrpGeom->DrawBoxA(x, y, x + 400, y + 32);
      GrpGeom->Unlock();
    }

    const auto &record = scores_[i];
    GrpPut16c2(x + 88, y + 4, record.name.c_str());
    const auto score = std::format("{:11}", record.score);
    GrpPut16c2(x + 216, y + 4, score.c_str());
    const auto graze = std::format("{:6}", record.graze);
    GrpPutScore(x + 120, y + 25, graze.c_str());
    if (record.stage == StageId::Extra) {
      GrpSurface_Blit({x + 224, y + 24}, SURFACE_ID::SYSTEM,
                      PIXEL_LTWH{288, 88, 16, 8});
    } else {
      const auto stage =
          std::format("{}", std::to_underlying(record.stage) + 1);
      GrpPutScore(x + 224, y + 25, stage.c_str());
    }
    GrpSurface_Blit(
        {x + 304, y + 24}, SURFACE_ID::NAMEREG,
        PIXEL_LTWH{0, 400 + std::to_underlying(record.player_type) * 8, 48, 8});
  }
  const auto difficulty =
      std::string{GameLevelName(static_cast<GameLevel>(current_difficulty_))};
  GrpPut16(320, 450, difficulty.c_str());
}

void ScoreScene::DrawDetail() const {
  const auto &record = scores_[selected_];
  constexpr int x = 80;
  constexpr int y = 92;
  constexpr int width = 480;
  constexpr int height = 268;
  GrpGeom->Lock();
  GrpGeom->SetAlphaNorm(224);
  GrpGeom->SetColor({0, 0, 1});
  GrpGeom->DrawBoxA(x, y, x + width, y + height);
  GrpGeom->SetAlphaNorm(255);
  GrpGeom->SetColor({4, 4, 5});
  GrpGeom->DrawBox(x, y, x + width, y + 1);
  GrpGeom->DrawBox(x, y + height - 1, x + width, y + height);
  GrpGeom->DrawBox(x, y, x + 1, y + height);
  GrpGeom->DrawBox(x + width - 1, y, x + width, y + height);
  GrpGeom->Unlock();

  constexpr int text_x = x + 20;
  int text_y = y + 12;
  GrpPut16(text_x + 144, text_y, "Score Detail");
  text_y += 28;
  const std::array lines = {
      std::format("Name       {}", record.name),
      std::format("Date       {}", RecordDate(record.created_at)),
      std::format("Difficulty {}", GameLevelName(record.difficulty)),
      std::format("Stage      {}", StageName(record.stage)),
      std::format("Weapon     {}", PlayerTypeName(record.player_type)),
      std::format("Score      {}", record.score),
      std::format("Graze      {}", record.graze),
      std::format("Miss       {}", record.miss_count),
      std::format("Bomb       {}", record.bomb_used),
      std::format("Deathbomb  {}", record.deathbomb_count),
  };
  for (const auto &line : lines) {
    GrpPut16(text_x, text_y, line.c_str());
    text_y += 21;
  }
}
