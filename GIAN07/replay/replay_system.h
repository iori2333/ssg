///
/// ReplaySystem - input recording, demo playback, and replay persistence
///

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "data/game_data.h"
#include "gameplay/game_rules.h"
#include "sys/input.h"

inline constexpr auto kReplayBufferCapacity = 60 * 60 * 30;
inline constexpr auto kReplayStageCapacity = 6;

struct ReplayConfig {
  uint8_t game_level;
  uint8_t player_stock;
  uint8_t bomb_stock;
  uint8_t padding_1[5] = {0};
  uint8_t input_flags;
  uint8_t padding_2[15] = {0};
};
static_assert(sizeof(ReplayConfig) == 24);

struct ReplayState {
  uint32_t random_seed;
  uint32_t frame_count;
  ReplayConfig config;
  uint8_t power;
  uint8_t weapon;
};
static_assert(sizeof(ReplayState) == 36);

struct ReplayArchiveHeader {
  uint32_t random_seed;
  uint8_t stage_count;
  ReplayConfig config;
  uint8_t power;
  uint8_t weapon;
  uint8_t stages[kReplayStageCapacity];
  uint32_t frame_counts[kReplayStageCapacity];
};
static_assert(sizeof(ReplayArchiveHeader) == 64);

class ReplaySystem {
public:
  explicit ReplaySystem(const data::GameData &data) : data_(data) {}

  void BeginRecording();
  [[nodiscard]] bool HasRecordedStages() const;
  void FlushStage();
  void Record(INPUT_BITS input);
  void UpdateLastRecordedInput(INPUT_BITS input);

  [[nodiscard]] bool LoadStageDemo(StageId stage);
  void SaveReplay(bool extra_stage);
  [[nodiscard]] bool LoadReplay(const char *path);

  [[nodiscard]] INPUT_BITS NextInput();
  void StopPlayback();
  void CancelRecording() { recording_ = false; }
  [[nodiscard]] bool IsPlaying() const { return playing_; }
  [[nodiscard]] bool IsRecording() const { return recording_; }
  [[nodiscard]] bool IsMultiStagePlayback() const {
    return multi_stage_playback_;
  }
  [[nodiscard]] StageId PlaybackLastStage() const {
    return playback_last_stage_;
  }
  [[nodiscard]] std::optional<StageId> FirstPlaybackStage() const;

  void QueuePlayback(std::string path) { pending_playback_ = std::move(path); }
  [[nodiscard]] std::string TakePendingPlayback();

private:
  struct SavedConfig {
    uint8_t player_stock = 0;
    uint8_t bomb_stock = 0;
    uint8_t input_flags = 0;
  } saved_config_;

  [[nodiscard]] bool PreparePlayback();

  const data::GameData &data_;
  bool playing_ = false;
  bool recording_ = false;
  bool multi_stage_playback_ = false;
  ReplayArchiveHeader archive_header_{};
  StageId playback_last_stage_ = StageId::Stage1;
  std::string pending_playback_;
  ReplayState state_{};
  std::array<INPUT_BITS, kReplayBufferCapacity> input_buffer_{};
  uint32_t frame_cursor_ = 0;
  std::vector<std::vector<INPUT_BITS>> recorded_stages_;
  uint8_t recorded_stage_count_ = 0;
  std::array<uint8_t, kReplayStageCapacity> recorded_stage_ids_{};
  std::array<uint32_t, kReplayStageCapacity> recorded_stage_frames_{};
  std::vector<INPUT_BITS> playback_buffer_;
};
