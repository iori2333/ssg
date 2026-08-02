/// Score leaderboard, detail display, and name registration UI scene.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "score_scene.h"

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
#include "gfx/text_ttf.h"
#include "i18n/localization.h"
#include "music/music_player.h"
#include "player/loadout/player_loadout.h"
#include "record/record_system.h"
#include "sys/input.h"
#include "sys/log.h"
#include "ui/name_entry.h"
#include "ui/ui_manager.h"
#include "util/time_api.h"

namespace {
constexpr auto kDefaultScoreName = "Vivit!";

std::string_view Text(const i18n::Localization &localization,
                      std::string_view key) {
  return localization.Text(i18n::TextIdFromKey(key));
}

std::string_view DifficultyName(const i18n::Localization &localization,
                                GameLevel level) {
  constexpr std::array keys = {"ui.value.easy", "ui.value.normal",
                               "ui.value.hard", "ui.value.lunatic",
                               "ui.value.extra"};
  const auto index = std::to_underlying(level);
  return index < keys.size() ? Text(localization, keys[index])
                             : std::string_view{};
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

std::string RecordDate(int64_t timestamp) {
  return std::format("{:%Y-%m-%d %H%M}",
                     util::LocalTime(util::UtcTime(timestamp)));
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

void RenderDetailRow(WindowPoint position, TextRenderRectId rect,
                     std::string_view label, std::string_view value) {
  const auto cache_key = std::format("{}\x1F{}", label, value);
  TextRenderer().Render(
      position, rect, cache_key, [label, value](TextRenderSession &s) {
        constexpr int label_right = 132;
        constexpr int separator_x = 144;
        constexpr int value_x = 168;
        constexpr Rgb shadow{.r = 64, .g = 64, .b = 80};
        constexpr Rgb label_color{.r = 128, .g = 180, .b = 255};
        constexpr Rgb value_color{.r = 255, .g = 255, .b = 255};

        s.SetFont(FontId::Normal);
        const auto label_x =
            std::max(0, label_right - TextRenderSession::Extent(label).w);
        s.Put({.x = label_x + 1, .y = 1}, label, shadow);
        s.Put({.x = label_x, .y = 0}, label, label_color);
        s.Put({.x = separator_x + 1, .y = 1}, ":", shadow);
        s.Put({.x = separator_x, .y = 0}, ":", label_color);
        s.Put({.x = value_x + 1, .y = 1}, value, shadow);
        s.Put({.x = value_x, .y = 0}, value, value_color);
      });
}
} // namespace

void ScoreScene::LoadLeaderboard(GameLevel difficulty) {
  scores_ = RecordSystem::ListScores(difficulty, kRowCount);
  selected_ = 0;
  detail_open_ = false;
  ResetRows();
}

bool ScoreScene::ShowLeaderboard(GameLevel initial_difficulty,
                                 InputBits initial_input) {
  current_difficulty_ = std::to_underlying(initial_difficulty);
  LoadLeaderboard(static_cast<GameLevel>(current_difficulty_));

  ui_.ForceCloseMessageWindow();
  GraphicsBackendClear();
  GraphicsFlip();
  if (!graphics_.LoadNameRegistration()) {
    logging::Error(logging::Channel::Ui,
                   "Failed to load Score screen graphics");
    return false;
  }

  GraphicsBackendSetClip(kGameResolutionRect);
  TextRenderer().Clear();
  ui_text_ = TextRenderer().Register({.w = 480, .h = 24});
  input_locked_ = initial_input != 0U;
  return true;
}

ScoreSceneResult ScoreScene::UpdateLeaderboard(InputBits input,
                                               bool should_draw) {
  if (input == 0U) {
    input_locked_ = false;
  } else if (!input_locked_) {
    input_locked_ = true;
    if (detail_open_) {
      if (InputIsOk(input) || InputIsCancel(input)) {
        detail_open_ = false;
        audio_.PlaySfx(SfxId::Cancel);
      }
    } else if (input == KeyEscape || input == KeyBomb) {
      audio_.PlaySfx(SfxId::Cancel);
      return ScoreSceneResult::ExitRequested;
    } else if (input == KeyUp && !scores_.empty()) {
      selected_ = (selected_ + scores_.size() - 1) % scores_.size();
      audio_.PlaySfx(SfxId::Select);
    } else if (input == KeyDown && !scores_.empty()) {
      selected_ = (selected_ + 1) % scores_.size();
      audio_.PlaySfx(SfxId::Select);
    } else if ((input == KeyLeft || input == KeyRight) &&
               !rows_.back().moving) {
      const auto level_count = kGameLevelNames.size();
      const auto direction = input == KeyLeft ? level_count - 1 : 1;
      current_difficulty_ = (current_difficulty_ + direction) % level_count;
      LoadLeaderboard(static_cast<GameLevel>(current_difficulty_));
      audio_.PlaySfx(SfxId::Select);
    } else if (InputIsOk(input) && !scores_.empty()) {
      detail_open_ = true;
      audio_.PlaySfx(SfxId::Select);
    }
  }

  if (should_draw) {
    DrawLeaderboard(true);
    if (detail_open_) {
      DrawDetail();
    }
    GraphicsFlip();
  }
  return ScoreSceneResult::Running;
}

ScoreRegistrationStart
ScoreScene::StartNameRegistration(ScoreRecord record, InputBits initial_input,
                                  bool change_music) {
  current_record_.emplace(std::move(record));
  scores_ = RecordSystem::ListScores(current_record_->difficulty, kRowCount);
  const auto position =
      std::ranges::find_if(scores_, [this](const ScoreRecord &record) {
        return current_record_->score > record.score;
      });
  pending_rank_ = static_cast<std::size_t>(position - scores_.begin());
  if (pending_rank_ >= kRowCount) {
    current_record_->name = kDefaultScoreName;
    if (RecordSystem::SaveScore(*current_record_) != RecordSaveResult::Saved) {
      logging::Error(logging::Channel::Record, "Score could not be saved");
    }
    current_record_.reset();
    return ScoreRegistrationStart::Complete;
  }
  scores_.insert(position, *current_record_);
  if (scores_.size() > kRowCount) {
    scores_.erase(scores_.begin() + static_cast<std::ptrdiff_t>(kRowCount),
                  scores_.end());
  }

  audio_.StopSfx(SfxId::Warning);
  audio_.StopAllSfx();
  ui_.ForceCloseMessageWindow();
  GraphicsBackendClear();
  GraphicsFlip();
  if (!graphics_.LoadNameRegistration()) {
    logging::Error(logging::Channel::Ui,
                   "Failed to load Score registration graphics");
    current_record_.reset();
    return ScoreRegistrationStart::Complete;
  }

  current_difficulty_ = std::to_underlying(current_record_->difficulty);
  ResetRows();
  GraphicsBackendSetClip(kGameResolutionRect);
  TextRenderer().Clear();
  ui_text_ = TextRenderer().Register({.w = 480, .h = 24});
  name_entry_.Begin(true, initial_input);
  save_failed_ = false;
  if (change_music) {
    music_.Play(19);
  }
  return ScoreRegistrationStart::Active;
}

ScoreSceneResult ScoreScene::UpdateNameRegistration(InputBits input,
                                                    bool should_draw) {
  const auto result = name_entry_.Update(input);
  scores_[pending_rank_].name = name_entry_.Name();
  if (result == NameEntryResult::Confirmed) {
    if (!current_record_) {
      return ScoreSceneResult::RegistrationComplete;
    }
    current_record_->name = name_entry_.Name();
    if (RecordSystem::SaveScore(*current_record_) == RecordSaveResult::Saved) {
      current_record_.reset();
      return ScoreSceneResult::RegistrationComplete;
    }
    save_failed_ = true;
    audio_.PlaySfx(SfxId::Cancel);
  } else if (result == NameEntryResult::Cancelled) {
    current_record_.reset();
    return ScoreSceneResult::RegistrationComplete;
  }

  if (should_draw) {
    DrawLeaderboard(false);
    const auto gx = rows_[pending_rank_].x >> 6;
    const auto gy = rows_[pending_rank_].y >> 6;
    name_entry_.Draw(gx + 88, gy + 4);
    if (save_failed_) {
      const auto failed = Text(localization_, "ui.score.save_failed");
      RenderUiText({80, 390}, ui_text_, failed, true);
    }
    GraphicsFlip();
  }
  return ScoreSceneResult::Running;
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
  GraphicsBackendClear();
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
    GraphicsSurfaceBlit({x, y}, SurfaceId::NameRegistration,
                        PixelLtwh{0, 64 + static_cast<int>(32 * i), 400, 32});
    if (i >= scores_.size()) {
      continue;
    }
    if (show_selection && i == selected_) {
      geometry::SetAlphaNorm(96);
      geometry::SetColor({4, 0, 0});
      geometry::DrawBoxA(x, y, x + 400, y + 32);
    }

    const auto &record = scores_[i];
    DrawFont16C2(x + 88, y + 4, record.name.c_str());
    const auto score = std::format("{:11}", record.score);
    DrawFont16C2(x + 216, y + 4, score.c_str());
    const auto graze = std::format("{:6}", record.graze);
    DrawScore(x + 120, y + 25, graze.c_str());
    if (record.stage == StageId::Extra) {
      GraphicsSurfaceBlit({x + 224, y + 24}, SurfaceId::System,
                          PixelLtwh{288, 88, 16, 8});
    } else {
      const auto stage =
          std::format("{}", std::to_underlying(record.stage) + 1);
      DrawScore(x + 224, y + 25, stage.c_str());
    }
    GraphicsSurfaceBlit(
        {x + 304, y + 24}, SurfaceId::NameRegistration,
        PixelLtwh{0, 400 + std::to_underlying(record.player_type) * 8, 48, 8});
  }
  const auto difficulty =
      std::string{GameLevelName(static_cast<GameLevel>(current_difficulty_))};
  DrawFont16(320, 450, difficulty.c_str());
}

void ScoreScene::DrawDetail() const {
  const auto &record = scores_[selected_];
  constexpr int x = 80;
  constexpr int y = 92;
  constexpr int width = 480;
  constexpr int height = 268;
  geometry::SetAlphaNorm(224);
  geometry::SetColor({0, 0, 1});
  geometry::DrawBoxA(x, y, x + width, y + height);
  geometry::SetAlphaNorm(255);
  geometry::SetColor({4, 4, 5});
  geometry::DrawBox(x, y, x + width, y + 1);
  geometry::DrawBox(x, y + height - 1, x + width, y + height);
  geometry::DrawBox(x, y, x + 1, y + height);
  geometry::DrawBox(x + width - 1, y, x + width, y + height);

  constexpr int text_x = x + 20;
  int text_y = y + 12;
  const auto detail_title = Text(localization_, "ui.score.detail");
  RenderUiText({x, text_y}, ui_text_, detail_title, true);
  text_y += 28;
  const std::array labels = {
      Text(localization_, "ui.score.name"),
      Text(localization_, "ui.score.date"),
      Text(localization_, "ui.score.difficulty"),
      Text(localization_, "ui.score.stage"),
      Text(localization_, "ui.score.weapon"),
      Text(localization_, "ui.score.score"),
      Text(localization_, "ui.score.graze"),
      Text(localization_, "ui.score.miss"),
      Text(localization_, "ui.score.bomb"),
      Text(localization_, "ui.score.deathbomb"),
  };
  const std::array values = {
      std::string(record.name),
      RecordDate(record.created_at),
      std::string(DifficultyName(localization_, record.difficulty)),
      std::string(StageName(localization_, record.stage)),
      std::string(PlayerName(localization_, record.player_type)),
      std::format("{}", record.score),
      std::format("{}", record.graze),
      std::format("{}", record.miss_count),
      std::format("{}", record.bomb_used),
      std::format("{}", record.deathbomb_count),
  };
  for (size_t i = 0; i < labels.size(); ++i) {
    RenderDetailRow({text_x, text_y}, ui_text_, labels[i], values[i]);
    text_y += 21;
  }
}
