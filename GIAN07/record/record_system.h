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
#include "util/enum_flags.h"
#include "util/math_utils.h"

namespace data {
class GameData;
class PbgArchive;
} // namespace data

struct ConfigData;
struct GameSession;

enum class ReplayInputFlags : uint8_t {
  None = 0,
  Joypad = 1 << 0,
  MessageSkip = 1 << 1,
  SpeedDown = 1 << 2,
};

template <> inline constexpr bool util::EnableEnumFlags<ReplayInputFlags> = true;

[[nodiscard]] inline constexpr bool
HasReplayInputFlag(ReplayInputFlags flags, ReplayInputFlags flag) {
  return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

inline constexpr auto kReplayBufferCapacity = 60 * 60 * 30;
inline constexpr auto kReplayStageCapacity = 6;
inline constexpr auto kRecordNameLength = 8;

struct ScoreRecord {
  std::string name;
  int64_t created_at;
  int64_t score;
  int graze;
  int miss_count;
  int bomb_used;
  int deathbomb_count;
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

  [[nodiscard]] static ScoreRecord CaptureScore(const Player &player,
                                                const GameSession &session);
  [[nodiscard]] static std::vector<ScoreRecord> ListScores(GameLevel difficulty,
                                                           std::size_t limit);
  [[nodiscard]] static RecordSaveResult SaveScore(const ScoreRecord &record);

  void BeginRecording(const Player &player, const GameSession &session,
                      const ConfigData &config);
  void BeginDemoCapture(const Player &player, const GameSession &session,
                        const ConfigData &config);
  [[nodiscard]] bool MarkDemoStart();
  void BeginStage(const Player &player, const GameSession &session);
  [[nodiscard]] bool HasRecordedStages() const;
  void FlushStage();
  void Record(InputBits input);
  void UpdateLastRecordedInput(InputBits input);
  void CancelRecording();
  [[nodiscard]] RecordSaveResult SaveReplay(std::string_view name,
                                            bool extra_stage);
  [[nodiscard]] RecordSaveResult SaveDemo(StageId stage);

  [[nodiscard]] static std::vector<ReplayRecord> ListReplays();
  [[nodiscard]] bool LoadReplay(std::string_view path, StageId start_stage);
  [[nodiscard]] bool ConfigurePlayback(Player &player, GameSession &session);
  void RestorePlaybackStage(Player &player, GameSession &session);
  [[nodiscard]] bool HasNextPlaybackStage() const;
  [[nodiscard]] bool AdvancePlaybackStage();
  [[nodiscard]] StageId CurrentPlaybackStage() const;

  [[nodiscard]] bool LoadStageDemo(StageId stage, Player &player,
                                   GameSession &session);
  [[nodiscard]] bool HasStageDemo(StageId stage) const;
  [[nodiscard]] InputBits NextInput();
  void StopPlayback();
  [[nodiscard]] bool IsPlaying() const;
  [[nodiscard]] bool IsRecording() const;
  [[nodiscard]] bool IsMultiStagePlayback() const;

private:
  struct ReplaySettings {
    GameLevel difficulty = GameLevel::Normal;
    PracticeMode practice_mode = PracticeMode::Off;
    int player_stock = 0;
    int bomb_stock = 0;
    ReplayInputFlags input_flags = ReplayInputFlags::None;
  };

  struct StageCheckpoint {
    StageId stage = StageId::Stage1;
    std::size_t frame_count = 0;
    math::RandomState rng{};
    PlayerProgress player{};
    int rank = 0;
  };

  struct ReplayStage {
    StageCheckpoint checkpoint;
    std::vector<InputBits> inputs;
  };

  struct IdleState {};

  struct RecordingState {
    ReplaySettings settings;
    StageCheckpoint current_checkpoint;
    bool has_current_checkpoint = false;
    bool demo_capture = false;
    std::optional<std::size_t> demo_start_frame;
    std::vector<InputBits> current_inputs;
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

  [[nodiscard]] static bool LoadArchive(std::string_view path,
                                        std::optional<ReplayRecord> *record,
                                        ReplaySettings *settings,
                                        std::vector<ReplayStage> *stages);
  [[nodiscard]] static bool LoadArchive(const data::PbgArchive &archive,
                                        std::optional<ReplayRecord> *record,
                                        ReplaySettings *settings,
                                        std::vector<ReplayStage> *stages);
  [[nodiscard]] static bool LoadDemoArchive(const data::PbgArchive &archive,
                                            StageId stage,
                                            ReplaySettings &settings,
                                            std::vector<ReplayStage> &stages);
  void BeginCapture(const Player &player, const GameSession &session,
                    const ConfigData &config, bool demo_capture);
  [[nodiscard]] RecordSaveResult SaveRecording(std::string_view name,
                                               std::string_view path,
                                               int64_t created_at);

  const data::GameData &data_;
  std::variant<IdleState, RecordingState, PlaybackState> state_;
};
