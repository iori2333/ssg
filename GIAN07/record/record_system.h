/// Game record persistence, Replay recording, and playback.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "gameplay/game_rules.h"
#include "player/player.h"
#include "sys/input.h"
#include "util/math_utils.h"

namespace data {
class GameData;
class PbgArchive;
} // namespace data

struct ConfigData;
struct GameSession;

inline constexpr auto kReplayBufferCapacity = 60 * 60 * 30;
inline constexpr auto kReplayStageCapacity = 6;
inline constexpr auto kRecordNameLength = 8;

struct ScoreRecord {
  std::string name;
  int64_t created_at;
  int64_t score;
  uint32_t graze;
  uint16_t miss_count;
  uint16_t bomb_used;
  uint16_t deathbomb_count;
  GameLevel difficulty;
  StageId stage;
  PlayerType player_type;
};

struct ReplayRecord {
  std::string path;
  std::string name;
  int64_t created_at;
  GameLevel difficulty;
  PlayerType player_type;
  std::vector<StageId> stages;
};

enum class RecordSaveResult : uint8_t {
  Saved,
  NoData,
  IoError,
};

class RecordSystem {
public:
  explicit RecordSystem(const data::GameData &data) : data_(data) {}

  [[nodiscard]] ScoreRecord CaptureScore(const Player &player,
                                         const GameSession &session) const;
  [[nodiscard]] std::vector<ScoreRecord> ListScores(GameLevel difficulty,
                                                    std::size_t limit) const;
  [[nodiscard]] RecordSaveResult SaveScore(const ScoreRecord &record) const;

  void BeginRecording(const Player &player, const GameSession &session,
                      const ConfigData &config);
  void BeginDemoCapture(const Player &player, const GameSession &session,
                        const ConfigData &config);
  [[nodiscard]] bool MarkDemoStart();
  void BeginStage(const Player &player, const GameSession &session);
  [[nodiscard]] bool HasRecordedStages() const;
  void FlushStage();
  void Record(INPUT_BITS input);
  void UpdateLastRecordedInput(INPUT_BITS input);
  void CancelRecording();
  [[nodiscard]] RecordSaveResult SaveReplay(std::string_view name,
                                            bool extra_stage);
  [[nodiscard]] RecordSaveResult SaveDemo(StageId stage);

  [[nodiscard]] std::vector<ReplayRecord> ListReplays() const;
  [[nodiscard]] bool LoadReplay(std::string_view path, StageId start_stage);
  [[nodiscard]] bool ConfigurePlayback(Player &player, GameSession &session);
  void RestorePlaybackStage(Player &player, GameSession &session);
  [[nodiscard]] bool HasNextPlaybackStage() const;
  [[nodiscard]] bool AdvancePlaybackStage();
  [[nodiscard]] StageId CurrentPlaybackStage() const;

  [[nodiscard]] bool LoadStageDemo(StageId stage, Player &player,
                                   GameSession &session);
  [[nodiscard]] bool HasStageDemo(StageId stage) const;
  [[nodiscard]] INPUT_BITS NextInput();
  void StopPlayback();
  [[nodiscard]] bool IsPlaying() const;
  [[nodiscard]] bool IsRecording() const;
  [[nodiscard]] bool IsMultiStagePlayback() const;

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
    math::RandomState rng{};
    PlayerProgress player{};
    int32_t rank = 0;
  };

  struct ReplayStage {
    StageCheckpoint checkpoint;
    std::vector<INPUT_BITS> inputs;
  };

  struct IdleState {};

  struct RecordingState {
    ReplaySettings settings;
    StageCheckpoint current_checkpoint;
    bool has_current_checkpoint = false;
    bool demo_capture = false;
    std::optional<std::size_t> demo_start_frame;
    std::vector<INPUT_BITS> current_inputs;
    std::vector<ReplayStage> stages;
  };

  struct PlaybackState {
    ReplaySettings settings;
    std::vector<ReplayStage> stages;
    std::size_t stage_index = 0;
    std::size_t frame_cursor = 0;
    bool active = false;
    bool multi_stage = false;
  };

  [[nodiscard]] bool LoadArchive(std::string_view path,
                                 std::optional<ReplayRecord> *record,
                                 ReplaySettings *settings,
                                 std::vector<ReplayStage> *stages) const;
  [[nodiscard]] bool LoadArchive(const data::PbgArchive &archive,
                                 std::optional<ReplayRecord> *record,
                                 ReplaySettings *settings,
                                 std::vector<ReplayStage> *stages) const;
  [[nodiscard]] bool LoadDemoArchive(const data::PbgArchive &archive,
                                     StageId stage, ReplaySettings &settings,
                                     std::vector<ReplayStage> &stages) const;
  void BeginCapture(const Player &player, const GameSession &session,
                    const ConfigData &config, bool demo_capture);
  [[nodiscard]] RecordSaveResult SaveRecording(std::string_view name,
                                               std::string_view path,
                                               int64_t created_at);

  const data::GameData &data_;
  std::variant<IdleState, RecordingState, PlaybackState> state_;
};
