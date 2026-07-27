/// Replay recording, playback, and persistence.

#include <algorithm>
#include <chrono>
#include <format>
#include <utility>

#include "replay_system.h"

#include "data/pbg_archive.h"
#include "gameflow/gameflow_manager.h"
#include "gameplay/game_session.h"
#include "player/player.h"
#include "settings/config.h"
#include "sys/input.h"
#include "util/ut_math.h"

namespace {
std::string ReplayFilename(bool extra_stage) {
  const auto *prefix = extra_stage ? "replay_ex" : "replay";
  const auto now = std::chrono::system_clock::now();
  const auto local = std::chrono::floor<std::chrono::seconds>(now);
  return std::format("{}_{:%Y%m%d_%H%M%S}.DAT", prefix, local);
}
} // namespace

void ReplaySystem::BeginRecording() {
  playing_ = false;
  multi_stage_playback_ = false;

  // Prepare random seed
  state_.random_seed =
      ((Cast::up<uint32_t>(rnd()) + 1U) * (Cast::up<uint32_t>(rnd()) + 1U));
  rnd_seed_set(state_.random_seed);

  state_.power = GameFlow.ctx.player.Power();
  state_.weapon = std::to_underlying(GameFlow.ctx.player.Type());
  state_.config.game_level = std::to_underlying(GameFlow.ctx.session.level);
  state_.config.player_stock = GameFlow.ctx.player.Lives();
  state_.config.bomb_stock = GameFlow.ctx.config.game.bomb_stock;
  state_.config.input_flags = GameFlow.ctx.config.input.PackFlags();

  frame_cursor_ = 0;
  recording_ = true;
  recorded_stage_count_ = 0;
  recorded_stages_.clear();
}

bool ReplaySystem::HasRecordedStages() const {
  return recording_ && (recorded_stage_count_ > 0 || frame_cursor_ > 0);
}

void ReplaySystem::FlushStage() {
  if (!recording_) {
    return;
  }
  if (frame_cursor_ == 0) {
    return;
  }

  if (recorded_stage_count_ < kReplayStageCapacity) {
    recorded_stage_ids_[recorded_stage_count_] =
        std::to_underlying(GameFlow.ctx.session.stage);
    recorded_stage_frames_[recorded_stage_count_] = frame_cursor_;

    std::vector<INPUT_BITS> stage_data(input_buffer_.data(),
                                       input_buffer_.data() + frame_cursor_);
    recorded_stages_.push_back(std::move(stage_data));

    recorded_stage_count_++;
  }

  frame_cursor_ = 0;
}

bool ReplaySystem::PreparePlayback() {
  frame_cursor_ = 0;
  playing_ = true;

  // Initialize config
  // Preserve current config
  saved_config_.player_stock = GameFlow.ctx.config.game.player_stock;
  saved_config_.bomb_stock = GameFlow.ctx.config.game.bomb_stock;
  saved_config_.input_flags = GameFlow.ctx.config.input.PackFlags();

  // Transfer recorded config
  GameFlow.ctx.config.game.bomb_stock = state_.config.bomb_stock;
  GameFlow.ctx.config.game.player_stock = state_.config.player_stock;
  GameFlow.ctx.config.input.UnpackFlags(state_.config.input_flags);
  GameFlow.ctx.session.level = static_cast<GameLevel>(state_.config.game_level);

  // Restore player stats
  GameFlow.ctx.player.ApplyReplayState(state_.weapon, state_.power,
                                       GameFlow.ctx.config.game.player_stock,
                                       GameFlow.ctx.config.game.bomb_stock);

  // Initialize random number
  // Sync random seed last
  rnd_seed_set(state_.random_seed);

  return true;
}

void ReplaySystem::Record(INPUT_BITS key) {
  if (!recording_) {
    return;
  }

  input_buffer_[frame_cursor_++] = key;

  // Buffer full or ESC pressed
  if ((frame_cursor_ == kReplayBufferCapacity) || ((key & KEY_ESC) != 0)) {
    frame_cursor_--;
    return;
  }
}

void ReplaySystem::UpdateLastRecordedInput(INPUT_BITS key) {
  if (recording_ && frame_cursor_ != 0U) {
    input_buffer_[frame_cursor_ - 1] = key;
  }
}

bool ReplaySystem::LoadStageDemo(StageId stage) {
  playing_ = false;
  multi_stage_playback_ = false;

  // Unpack
  const auto temp = data_.ExtractMap(std::to_underlying(stage) + 6);
  auto temp_cursor = temp.cursor();
  {
    const auto maybe_info = temp_cursor.next<ReplayState>();
    if (!maybe_info) {
      return false;
    }
    state_ = maybe_info.value()[0];
  }
  {
    if (state_.frame_count > input_buffer_.size()) {
      return false;
    }
    const auto maybe_inputs = temp_cursor.next<uint16_t>(state_.frame_count);
    if (!maybe_inputs) {
      return false;
    }
    const auto inputs = maybe_inputs.value();
    memcpy(input_buffer_.data(), inputs.data(), inputs.size_bytes());
  }
  return PreparePlayback();
}
INPUT_BITS ReplaySystem::NextInput() {
  if (!playing_) {
    return KEY_ESC;
  }

  const auto ptr = frame_cursor_;
  if (ptr >= state_.frame_count) {
    playing_ = false;
    return KEY_ESC;
  }

  frame_cursor_++;
  return input_buffer_[ptr];
}

void ReplaySystem::StopPlayback() {
  GameFlow.ctx.config.game.player_stock = saved_config_.player_stock;
  GameFlow.ctx.config.game.bomb_stock = saved_config_.bomb_stock;
  GameFlow.ctx.config.input.UnpackFlags(saved_config_.input_flags);

  playing_ = false;
  multi_stage_playback_ = false;
}

void ReplaySystem::SaveReplay(bool extra_stage) {
  if (!recording_) {
    return;
  }

  // Flush current stage data if any (not yet flushed by stage clear)
  if (frame_cursor_ > 0 && recorded_stage_count_ < kReplayStageCapacity) {
    recorded_stage_ids_[recorded_stage_count_] =
        std::to_underlying(GameFlow.ctx.session.stage);
    recorded_stage_frames_[recorded_stage_count_] = frame_cursor_;
    recorded_stages_.emplace_back(input_buffer_.data(),
                                  input_buffer_.data() + frame_cursor_);
    recorded_stage_count_++;
    frame_cursor_ = 0;
  }

  recording_ = false;

  ReplayArchiveHeader info = {};
  info.random_seed = state_.random_seed;
  info.stage_count = recorded_stage_count_;
  info.config = state_.config;
  info.power = state_.power;
  info.weapon = state_.weapon;
  for (int i = 0; std::cmp_less(i, recorded_stage_count_); i++) {
    info.stages[i] = recorded_stage_ids_[i];
    info.frame_counts[i] = recorded_stage_frames_[i];
  }

  data::PbgArchiveWriter out;
  out.Add(std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(&info),
                                   sizeof(info)));
  for (auto &buf : recorded_stages_) {
    out.Add(
        std::span<const uint8_t>(reinterpret_cast<const uint8_t *>(buf.data()),
                                 buf.size() * sizeof(INPUT_BITS)));
  }

  const auto fn = ReplayFilename(extra_stage);
  out.Write(fn.c_str());

  recorded_stages_.clear();
  recorded_stage_count_ = 0;
}

bool ReplaySystem::LoadReplay(const char *fn) {
  playing_ = false;
  multi_stage_playback_ = false;

  const auto in = data::PbgArchive::Open(fn);
  if (!in) {
    return false;
  }

  BYTE_BUFFER_OWNED temp = in.Extract(0);
  if (temp.size() < sizeof(ReplayArchiveHeader)) {
    return false;
  }
  memcpy(&archive_header_, temp.get(), sizeof(ReplayArchiveHeader));
  if (archive_header_.stage_count == 0 ||
      archive_header_.stage_count > kReplayStageCapacity) {
    return false;
  }

  // Old replay files stored 1-based stage numbers (1-6 for normal, 131 for
  // EXTRA). Convert to new 0-based StageId format on load.
  if (archive_header_.stage_count > 0) {
    auto first = archive_header_.stages[0];
    if (first >= 1 && first <= kRegularStageCount) {
      for (uint8_t i = 0; i < archive_header_.stage_count; i++) {
        archive_header_.stages[i]--;
      }
    } else if (first > kRegularStageCount) {
      archive_header_.stages[0] = std::to_underlying(StageId::Extra);
    }
  }

  // Compute max stage for stage transition gating
  playback_last_stage_ = StageId::Stage1;
  for (uint8_t i = 0; i < archive_header_.stage_count; i++) {
    playback_last_stage_ = std::max(
        static_cast<StageId>(archive_header_.stages[i]), playback_last_stage_);
  }

  playback_buffer_.clear();
  uint32_t total_frames = 0;
  for (uint8_t i = 0; i < archive_header_.stage_count; i++) {
    temp = in.Extract(i + 1);
    const auto n_frames = archive_header_.frame_counts[i];
    if (n_frames > kReplayBufferCapacity - total_frames ||
        temp.size() < n_frames * sizeof(INPUT_BITS)) {
      return false;
    }
    const auto *const src = reinterpret_cast<const INPUT_BITS *>(temp.get());
    playback_buffer_.insert(playback_buffer_.end(), src, src + n_frames);
    total_frames += n_frames;
  }

  // Playback consumes one continuous input stream across stage transitions.
  memcpy(input_buffer_.data(), playback_buffer_.data(),
         total_frames * sizeof(INPUT_BITS));
  state_.frame_count = total_frames;
  state_.random_seed = archive_header_.random_seed;
  state_.config = archive_header_.config;
  state_.power = archive_header_.power;
  state_.weapon = archive_header_.weapon;

  multi_stage_playback_ = true;
  return PreparePlayback();
}

std::optional<StageId> ReplaySystem::FirstPlaybackStage() const {
  if (archive_header_.stage_count == 0) {
    return std::nullopt;
  }
  return static_cast<StageId>(archive_header_.stages[0]);
}

std::string ReplaySystem::TakePendingPlayback() {
  return std::exchange(pending_playback_, {});
}
