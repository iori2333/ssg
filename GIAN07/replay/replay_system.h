/// Replay recording, catalog, persistence, and playback.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "gameplay/game_rules.h"
#include "player/player.h"
#include "sys/input.h"
#include "util/ut_math.h"

namespace data {
class GameData;
}

struct ConfigData;
struct GameSession;

inline constexpr auto kReplayBufferCapacity = 60 * 60 * 30;
inline constexpr auto kReplayStageCapacity = 6;
inline constexpr auto kReplayNameLength = 8;

struct ReplayMetadata {
  std::string path;
  std::string name;
  int64_t created_at = 0;
  GameLevel difficulty = GameLevel::Normal;
  PlayerType player_type = PlayerType::Wide;
  std::vector<StageId> stages;
};

class ReplaySystem {
public:
  explicit ReplaySystem(const data::GameData &data) : data_(data) {}

  void BeginRecording(const Player &player, const GameSession &session,
                      const ConfigData &config);
  void BeginStage(const Player &player, const GameSession &session);
  [[nodiscard]] bool HasRecordedStages() const;
  void FlushStage();
  void Record(INPUT_BITS input);
  void UpdateLastRecordedInput(INPUT_BITS input);
  void CancelRecording();
  [[nodiscard]] bool SaveReplay(std::string_view name, bool extra_stage);

  [[nodiscard]] std::vector<ReplayMetadata> ListReplays() const;
  [[nodiscard]] bool LoadReplay(std::string_view path, StageId start_stage);
  [[nodiscard]] bool ConfigurePlayback(ConfigData &config,
                                       GameSession &session);
  void RestorePlaybackStage(Player &player, GameSession &session);
  [[nodiscard]] bool HasNextPlaybackStage() const;
  [[nodiscard]] bool AdvancePlaybackStage();
  [[nodiscard]] StageId CurrentPlaybackStage() const;

  [[nodiscard]] bool LoadStageDemo(StageId stage);
  [[nodiscard]] INPUT_BITS NextInput();
  void StopPlayback(ConfigData &config, GameSession &session);
  [[nodiscard]] bool IsPlaying() const { return playing_; }
  [[nodiscard]] bool IsRecording() const { return recording_; }
  [[nodiscard]] bool IsMultiStagePlayback() const {
    return multi_stage_playback_;
  }

private:
  struct ReplaySettings {
    GameLevel difficulty = GameLevel::Normal;
    PracticeMode practice_mode = PracticeMode::Off;
    uint8_t player_stock = 0;
    uint8_t bomb_stock = 0;
    uint8_t input_flags = 0;
  };

  struct StageCheckpoint {
    StageId stage = StageId::Stage1;
    uint32_t frame_count = 0;
    RandomState rng{};
    PlayerProgress player{};
    int32_t rank = 0;
  };

  struct ReplayStage {
    StageCheckpoint checkpoint;
    std::vector<INPUT_BITS> inputs;
  };

  struct SavedConfig {
    GameLevel difficulty = GameLevel::Normal;
    PracticeMode practice_mode = PracticeMode::Off;
    uint8_t player_stock = 0;
    uint8_t bomb_stock = 0;
    uint8_t input_flags = 0;
  } saved_config_;

  [[nodiscard]] bool LoadArchive(std::string_view path,
                                 ReplayMetadata *metadata,
                                 ReplaySettings *settings,
                                 std::vector<ReplayStage> *stages) const;

  const data::GameData &data_;
  ReplaySettings settings_{};
  StageCheckpoint current_checkpoint_{};
  bool has_current_checkpoint_ = false;
  bool playing_ = false;
  bool recording_ = false;
  bool multi_stage_playback_ = false;
  bool config_overridden_ = false;
  std::vector<INPUT_BITS> current_inputs_;
  std::vector<ReplayStage> recorded_stages_;
  std::vector<ReplayStage> playback_stages_;
  std::size_t playback_stage_index_ = 0;
  std::size_t frame_cursor_ = 0;
};
