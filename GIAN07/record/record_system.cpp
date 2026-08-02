/// Game record persistence, Replay recording, and playback.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <ios>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "record_system.h"

#include "data/game_data.h"
#include "data/pbg_archive.h"
#include "gameplay/game_rules.h"
#include "gameplay/game_session.h"
#include "player/loadout/player_loadout.h"
#include "player/player.h"
#include "settings/config.h"
#include "sys/input.h"
#include "sys/log.h"
#include "util/byte_io.h"
#include "util/endian.h"
#include "util/math_utils.h"

namespace {

constexpr std::array<uint8_t, 4> kReplayMagic = {'G', '7', 'R', 'P'};
constexpr uint16_t kReplayVersion = 2;
constexpr auto kReplayDirectory = "replays";
constexpr auto kDemoDirectory = "demos";
constexpr std::array<uint8_t, 4> kScoreMagic = {'G', '7', 'S', 'C'};
constexpr uint16_t kScoreVersion = 1;
constexpr auto kScoreDirectory = "scores";
constexpr auto kMaxSignedRecordValue =
    static_cast<uint64_t>(std::numeric_limits<int64_t>::max());
constexpr uint8_t kInputFlagJoypad = 1 << 0;
constexpr uint8_t kInputFlagMessageSkip = 1 << 1;
constexpr uint8_t kInputFlagSpeedDown = 1 << 2;

struct ScoreRecordFile {
  std::array<uint8_t, 4> magic{};
  util::LittleEndian<uint16_t> version = 0;
  util::LittleEndian<uint64_t> created_at = 0;
  util::LittleEndian<uint64_t> score = 0;
  util::LittleEndian<uint32_t> graze = 0;
  util::LittleEndian<uint16_t> miss_count = 0;
  util::LittleEndian<uint16_t> bomb_used = 0;
  util::LittleEndian<uint16_t> deathbomb_count = 0;
  uint8_t difficulty = 0;
  uint8_t stage = 0;
  uint8_t player_type = 0;
  uint8_t name_length = 0;
  std::array<char, kRecordNameLength> name{};
};
static_assert(sizeof(ScoreRecordFile) == 44);

template <typename T> bool ReadValue(util::ByteReader &reader, T &value) {
  using U = std::make_unsigned_t<T>;
  const auto encoded = reader.Read<U>();
  if (!encoded) {
    return false;
  }
  value = static_cast<T>(*encoded);
  return true;
}

void WritePlayerProgress(util::ByteWriter &writer,
                         const PlayerProgress &progress) {
  writer.Write<uint64_t>(static_cast<uint64_t>(progress.score));
  writer.Write<uint64_t>(static_cast<uint64_t>(progress.pending_score));
  writer.Write(progress.graze_sum);
  writer.Write<uint32_t>(static_cast<uint32_t>(progress.pending_graze_score));
  writer.Write(progress.star_counter);
  writer.Write(progress.star_threshold);
  writer.Write(progress.graze_count);
  writer.Write(progress.graze_wait);
  writer.Write(progress.power_progress);
  writer.Write(progress.miss_count);
  writer.Write(progress.bomb_used);
  writer.Write(progress.deathbomb_count);
  writer.Write(progress.player_type);
  writer.Write(progress.power);
  writer.Write(progress.bombs);
  writer.Write(progress.lives);
  writer.Write(progress.credits);
  writer.Write(progress.star_extend_count);
  writer.Write(progress.initial_bomb_stock);
}

bool ReadPlayerProgress(util::ByteReader &reader, PlayerProgress &progress) {
  return ReadValue(reader, progress.score) &&
         ReadValue(reader, progress.pending_score) &&
         ReadValue(reader, progress.graze_sum) &&
         ReadValue(reader, progress.pending_graze_score) &&
         ReadValue(reader, progress.star_counter) &&
         ReadValue(reader, progress.star_threshold) &&
         ReadValue(reader, progress.graze_count) &&
         ReadValue(reader, progress.graze_wait) &&
         ReadValue(reader, progress.power_progress) &&
         ReadValue(reader, progress.miss_count) &&
         ReadValue(reader, progress.bomb_used) &&
         ReadValue(reader, progress.deathbomb_count) &&
         ReadValue(reader, progress.player_type) &&
         ReadValue(reader, progress.power) &&
         ReadValue(reader, progress.bombs) &&
         ReadValue(reader, progress.lives) &&
         ReadValue(reader, progress.credits) &&
         ReadValue(reader, progress.star_extend_count) &&
         ReadValue(reader, progress.initial_bomb_stock);
}

std::string ReplayFilename(bool extra_stage,
                           std::chrono::system_clock::time_point now) {
  const auto *prefix = extra_stage ? "replay_ex" : "replay";
  return std::format("{}_{:%Y%m%d_%H%M%S}.dat", prefix,
                     std::chrono::floor<std::chrono::seconds>(now));
}

std::optional<std::filesystem::path>
UniqueReplayPath(bool extra_stage, std::chrono::system_clock::time_point now) {
  std::error_code error;
  const auto filename = ReplayFilename(extra_stage, now);
  const auto stem = std::filesystem::path{filename}.stem().string();
  auto path = std::filesystem::path{kReplayDirectory} / filename;
  for (uint32_t suffix = 1; std::filesystem::exists(path, error); ++suffix) {
    if (error) {
      return std::nullopt;
    }
    path = std::filesystem::path{kReplayDirectory} /
           std::format("{}_{}.dat", stem, suffix);
  }
  return error ? std::nullopt : std::optional{std::move(path)};
}

bool IsReplayFilename(std::string_view name) {
  const auto replay_prefix = name.starts_with("replay_");
  const auto extra_prefix = name.starts_with("replay_ex_");
  const auto extension = std::filesystem::path{name}.extension().string();
  return (replay_prefix || extra_prefix) && extension == ".dat";
}

std::filesystem::path DemoPath(StageId stage) {
  return std::filesystem::path{kDemoDirectory} /
         std::format("{:03}.dat", std::to_underlying(stage));
}

bool IsScoreFilename(const std::filesystem::path &path) {
  const auto filename = path.filename().string();
  return filename.starts_with("score_") && path.extension().string() == ".dat";
}

std::optional<ScoreRecord> LoadScoreRecord(const std::filesystem::path &path) {
  std::error_code error;
  if (std::filesystem::file_size(path, error) != sizeof(ScoreRecordFile) ||
      error) {
    return std::nullopt;
  }
  ScoreRecordFile file;
  std::ifstream stream(path, std::ios::binary);
  stream.read(reinterpret_cast<char *>(&file), sizeof(file));
  if (!stream || file.magic != kScoreMagic || file.version != kScoreVersion ||
      file.created_at > kMaxSignedRecordValue ||
      file.score > kMaxSignedRecordValue ||
      file.difficulty > std::to_underlying(GameLevel::Extra) ||
      file.stage > std::to_underlying(StageId::Extra) ||
      file.player_type > std::to_underlying(PlayerType::Laser) ||
      file.name_length > kRecordNameLength) {
    return std::nullopt;
  }

  return ScoreRecord{
      .name = std::string{file.name.data(), file.name_length},
      .created_at = static_cast<int64_t>(file.created_at),
      .score = static_cast<int64_t>(file.score),
      .graze = file.graze,
      .miss_count = file.miss_count,
      .bomb_used = file.bomb_used,
      .deathbomb_count = file.deathbomb_count,
      .difficulty = static_cast<GameLevel>(file.difficulty),
      .stage = static_cast<StageId>(file.stage),
      .player_type = static_cast<PlayerType>(file.player_type),
  };
}

} // namespace

ScoreRecord RecordSystem::CaptureScore(const Player &player,
                                       const GameSession &session) {
  const auto now = std::chrono::system_clock::now();
  return ScoreRecord{
      .name = "",
      .created_at = std::chrono::duration_cast<std::chrono::seconds>(
                        now.time_since_epoch())
                        .count(),
      .score = player.Score(),
      .graze = player.GrazeSum(),
      .miss_count = player.MissCount(),
      .bomb_used = player.BombUsed(),
      .deathbomb_count = player.DeathbombCount(),
      .difficulty =
          session.stage == StageId::Extra ? GameLevel::Extra : session.level,
      .stage = session.stage,
      .player_type = player.Type(),
  };
}

std::vector<ScoreRecord> RecordSystem::ListScores(GameLevel difficulty,
                                                  std::size_t limit) {
  std::vector<ScoreRecord> records;
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator{kScoreDirectory, error}) {
    if (error || !entry.is_regular_file(error) ||
        !IsScoreFilename(entry.path())) {
      continue;
    }
    auto record = LoadScoreRecord(entry.path());
    if (record && record->difficulty == difficulty) {
      records.push_back(std::move(*record));
    }
  }
  std::ranges::sort(records,
                    [](const ScoreRecord &left, const ScoreRecord &right) {
                      if (left.score != right.score) {
                        return left.score > right.score;
                      }
                      return left.created_at < right.created_at;
                    });
  if (records.size() > limit) {
    records.erase(records.begin() + static_cast<std::ptrdiff_t>(limit),
                  records.end());
  }
  return records;
}

RecordSaveResult RecordSystem::SaveScore(const ScoreRecord &record) {
  std::error_code error;
  std::filesystem::create_directories(kScoreDirectory, error);
  if (error) {
    return RecordSaveResult::IoError;
  }

  const auto now = std::chrono::system_clock::now();
  const auto file_id = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now.time_since_epoch())
                           .count();
  auto path = std::filesystem::path{kScoreDirectory} /
              std::format("score_{}.dat", file_id);
  for (uint32_t suffix = 1; std::filesystem::exists(path, error); suffix++) {
    if (error) {
      return RecordSaveResult::IoError;
    }
    path = std::filesystem::path{kScoreDirectory} /
           std::format("score_{}_{}.dat", file_id, suffix);
  }
  if (error) {
    return RecordSaveResult::IoError;
  }

  ScoreRecordFile file{
      .magic = kScoreMagic,
      .version = kScoreVersion,
      .created_at = static_cast<uint64_t>(record.created_at),
      .score = static_cast<uint64_t>(record.score),
      .graze = record.graze,
      .miss_count = record.miss_count,
      .bomb_used = record.bomb_used,
      .deathbomb_count = record.deathbomb_count,
      .difficulty = std::to_underlying(record.difficulty),
      .stage = std::to_underlying(record.stage),
      .player_type = std::to_underlying(record.player_type),
      .name_length = static_cast<uint8_t>(
          std::min(record.name.size(), std::size_t{kRecordNameLength})),
  };
  std::ranges::copy_n(record.name.begin(), file.name_length, file.name.begin());

  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream) {
    return RecordSaveResult::IoError;
  }
  stream.write(reinterpret_cast<const char *>(&file), sizeof(file));
  stream.close();
  return stream ? RecordSaveResult::Saved : RecordSaveResult::IoError;
}

void RecordSystem::BeginRecording(const Player &player,
                                  const GameSession &session,
                                  const ConfigData &config) {
  BeginCapture(player, session, config, false);
}

void RecordSystem::BeginDemoCapture(const Player &player,
                                    const GameSession &session,
                                    const ConfigData &config) {
  BeginCapture(player, session, config, true);
}

void RecordSystem::BeginCapture(const Player &player,
                                const GameSession &session,
                                const ConfigData &config, bool demo_capture) {
  const auto seed = ((static_cast<uint32_t>(math::RandomInt()) + 1U) *
                     (static_cast<uint32_t>(math::RandomInt()) + 1U));
  math::SeedRandom(seed);

  state_.emplace<RecordingState>(RecordingState{
      .settings =
          {
              .difficulty = session.level,
              .practice_mode = config.game.practice_mode,
              .player_stock = config.game.player_stock,
              .bomb_stock = config.game.bomb_stock,
              .input_flags = static_cast<uint8_t>(
                  (config.input.joypad_enabled ? kInputFlagJoypad : 0) |
                  (config.input.z_msg_skip_enabled ? kInputFlagMessageSkip
                                                   : 0) |
                  (config.input.z_spd_down_enabled ? kInputFlagSpeedDown : 0)),
          },
      .demo_capture = demo_capture,
  });
  BeginStage(player, session);
}

bool RecordSystem::MarkDemoStart() {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || !recording->demo_capture ||
      recording->demo_start_frame ||
      recording->current_inputs.size() >= kReplayBufferCapacity) {
    return false;
  }
  recording->demo_start_frame = recording->current_inputs.size();
  return true;
}

void RecordSystem::BeginStage(const Player &player,
                              const GameSession &session) {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr) {
    return;
  }
  recording->current_checkpoint = {
      .stage = session.stage,
      .frame_count = 0,
      .rng = math::CaptureRandomState(),
      .player = player.CaptureProgress(),
      .rank = session.Rank(),
  };
  recording->has_current_checkpoint = true;
  recording->current_inputs.clear();
  if (recording->demo_capture) {
    recording->stages.clear();
    recording->demo_start_frame.reset();
  }
}

bool RecordSystem::HasRecordedStages() const {
  const auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr) {
    return false;
  }
  if (!recording->demo_capture) {
    return !recording->stages.empty() || !recording->current_inputs.empty();
  }
  return recording->demo_start_frame &&
         recording->current_inputs.size() > *recording->demo_start_frame;
}

void RecordSystem::FlushStage() {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || !recording->has_current_checkpoint ||
      recording->current_inputs.empty()) {
    return;
  }
  if (recording->stages.size() >= kReplayStageCapacity) {
    return;
  }

  recording->current_checkpoint.frame_count =
      static_cast<uint32_t>(recording->current_inputs.size());
  recording->stages.push_back({.checkpoint = recording->current_checkpoint,
                               .inputs = std::move(recording->current_inputs)});
  recording->current_inputs.clear();
  recording->has_current_checkpoint = false;
}

void RecordSystem::Record(InputBits input) {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || (input & KeyEscape) != 0 ||
      recording->current_inputs.size() >= kReplayBufferCapacity) {
    return;
  }
  input &= ~KeyDemoStart;
  if (recording->demo_capture && recording->demo_start_frame &&
      recording->current_inputs.size() == *recording->demo_start_frame) {
    input |= KeyDemoStart;
  }
  recording->current_inputs.push_back(input);
}

void RecordSystem::UpdateLastRecordedInput(InputBits input) {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording != nullptr && !recording->current_inputs.empty()) {
    const auto demo_marker = recording->current_inputs.back() & KeyDemoStart;
    recording->current_inputs.back() = (input & ~KeyDemoStart) | demo_marker;
  }
}

void RecordSystem::CancelRecording() {
  if (std::holds_alternative<RecordingState>(state_)) {
    state_.emplace<IdleState>();
  }
}

RecordSaveResult RecordSystem::SaveReplay(std::string_view name,
                                          bool extra_stage) {
  const auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || recording->demo_capture) {
    return RecordSaveResult::NoData;
  }
  FlushStage();
  recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || recording->stages.empty()) {
    return RecordSaveResult::NoData;
  }

  std::string replay_name{name.substr(0, kRecordNameLength)};
  if (replay_name.empty()) {
    replay_name = "Vivit!";
  }

  const auto now = std::chrono::system_clock::now();
  const auto created_at =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();

  std::error_code error;
  std::filesystem::create_directories(kReplayDirectory, error);
  const auto replay_path = UniqueReplayPath(extra_stage, now);
  if (error || !replay_path) {
    return RecordSaveResult::IoError;
  }
  return SaveRecording(replay_name, replay_path->string(), created_at);
}

RecordSaveResult RecordSystem::SaveDemo(StageId stage) {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || !recording->demo_capture ||
      !recording->demo_start_frame) {
    return RecordSaveResult::NoData;
  }
  FlushStage();
  recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || recording->stages.size() != 1 ||
      recording->stages.front().checkpoint.stage != stage ||
      !std::ranges::any_of(recording->stages.front().inputs, [](auto input) {
        return (input & KeyDemoStart) != 0;
      })) {
    return RecordSaveResult::NoData;
  }

  const auto now = std::chrono::system_clock::now();
  const auto created_at =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();
  std::error_code error;
  std::filesystem::create_directories(kDemoDirectory, error);
  if (error) {
    return RecordSaveResult::IoError;
  }
  return SaveRecording("DEMO", DemoPath(stage).string(), created_at);
}

RecordSaveResult RecordSystem::SaveRecording(std::string_view replay_name,
                                             std::string_view path,
                                             int64_t created_at) {
  auto *recording = std::get_if<RecordingState>(&state_);
  if (recording == nullptr || recording->stages.empty() ||
      replay_name.size() > kRecordNameLength || created_at < 0) {
    return RecordSaveResult::NoData;
  }

  util::ByteWriter manifest;
  manifest.WriteBytes(kReplayMagic);
  manifest.Write(kReplayVersion);
  manifest.Write<uint64_t>(static_cast<uint64_t>(created_at));
  manifest.Write(std::to_underlying(recording->settings.difficulty));
  manifest.Write(std::to_underlying(recording->settings.practice_mode));
  manifest.Write(recording->settings.player_stock);
  manifest.Write(recording->settings.bomb_stock);
  manifest.Write(recording->settings.input_flags);
  manifest.Write(static_cast<uint8_t>(replay_name.size()));
  manifest.WriteBytes({reinterpret_cast<const uint8_t *>(replay_name.data()),
                       replay_name.size()});
  manifest.Write(static_cast<uint8_t>(recording->stages.size()));

  for (const auto &stage : recording->stages) {
    const auto &checkpoint = stage.checkpoint;
    manifest.Write(std::to_underlying(checkpoint.stage));
    manifest.Write(checkpoint.frame_count);
    manifest.Write(checkpoint.rng.seed);
    manifest.Write(checkpoint.rng.draw_count);
    manifest.Write<uint32_t>(static_cast<uint32_t>(checkpoint.rank));
    WritePlayerProgress(manifest, checkpoint.player);
  }

  data::PbgArchiveWriter archive;
  archive.Add(manifest.Bytes());
  for (const auto &stage : recording->stages) {
    util::ByteWriter inputs;
    for (const auto input : stage.inputs) {
      inputs.Write(static_cast<uint16_t>(input));
    }
    archive.Add(inputs.Bytes());
  }

  if (!archive.Write(std::filesystem::path{path})) {
    logging::Error(logging::Channel::Record,
                   "Failed to write replay archive: {}", path);
    return RecordSaveResult::IoError;
  }
  state_.emplace<IdleState>();
  logging::Info(logging::Channel::Record, "Saved replay archive: {}", path);
  return RecordSaveResult::Saved;
}

std::vector<ReplayRecord> RecordSystem::ListReplays() {
  std::vector<ReplayRecord> replays;
  std::error_code error;
  for (const auto &entry :
       std::filesystem::directory_iterator{kReplayDirectory, error}) {
    if (error || !entry.is_regular_file(error)) {
      continue;
    }
    const auto filename = entry.path().filename().string();
    if (!IsReplayFilename(filename)) {
      continue;
    }
    std::optional<ReplayRecord> record;
    const auto path = entry.path().string();
    if (LoadArchive(path, &record, nullptr, nullptr)) {
      replays.push_back(std::move(*record));
    }
  }
  std::ranges::sort(replays, std::greater{}, &ReplayRecord::created_at);
  return replays;
}

bool RecordSystem::LoadArchive(std::string_view path,
                               std::optional<ReplayRecord> *record,
                               ReplaySettings *settings,
                               std::vector<ReplayStage> *stages) {
  const auto archive = data::PbgArchive::Open(path);
  if (!archive) {
    return false;
  }
  if (!LoadArchive(archive, record, settings, stages)) {
    return false;
  }
  if (record != nullptr && record->has_value()) {
    record->value().path = path;
  }
  return true;
}

bool RecordSystem::LoadArchive(const data::PbgArchive &archive,
                               std::optional<ReplayRecord> *record,
                               ReplaySettings *settings,
                               std::vector<ReplayStage> *stages) {
  const auto manifest_data = archive.Extract(0);
  if (manifest_data.empty()) {
    return false;
  }
  util::ByteReader reader{manifest_data};
  const auto magic = reader.ReadBytes(kReplayMagic.size());
  uint16_t version = 0;
  uint64_t created_at = 0;
  uint8_t difficulty = 0;
  uint8_t practice_mode = 0;
  uint8_t player_stock = 0;
  uint8_t bomb_stock = 0;
  uint8_t input_flags = 0;
  uint8_t name_length = 0;
  if (!magic || !std::ranges::equal(*magic, kReplayMagic) ||
      !ReadValue(reader, version) || version != kReplayVersion ||
      !ReadValue(reader, created_at) || created_at > kMaxSignedRecordValue ||
      !ReadValue(reader, difficulty) || !ReadValue(reader, practice_mode) ||
      !ReadValue(reader, player_stock) || !ReadValue(reader, bomb_stock) ||
      !ReadValue(reader, input_flags) || !ReadValue(reader, name_length) ||
      name_length > kRecordNameLength) {
    return false;
  }
  const auto name_bytes = reader.ReadBytes(name_length);
  uint8_t stage_count = 0;
  if (!name_bytes || !ReadValue(reader, stage_count) || stage_count == 0 ||
      stage_count > kReplayStageCapacity ||
      archive.EntryCount() != static_cast<uint32_t>(stage_count + 1) ||
      difficulty > std::to_underlying(GameLevel::Extra) ||
      practice_mode > std::to_underlying(PracticeMode::Invincible)) {
    return false;
  }

  std::vector<StageId> stage_ids;
  stage_ids.reserve(stage_count);
  std::optional<PlayerType> player_type;

  std::vector<ReplayStage> parsed_stages;
  parsed_stages.reserve(stage_count);
  for (uint8_t i = 0; i < stage_count; i++) {
    StageCheckpoint checkpoint;
    uint8_t stage_id = 0;
    uint32_t rank = 0;
    if (!ReadValue(reader, stage_id) ||
        stage_id > std::to_underlying(StageId::Extra) ||
        !ReadValue(reader, checkpoint.frame_count) ||
        checkpoint.frame_count > kReplayBufferCapacity ||
        !ReadValue(reader, checkpoint.rng.seed) ||
        !ReadValue(reader, checkpoint.rng.draw_count) ||
        !ReadValue(reader, rank) ||
        !ReadPlayerProgress(reader, checkpoint.player) ||
        checkpoint.player.player_type > std::to_underlying(PlayerType::Laser)) {
      return false;
    }
    checkpoint.stage = static_cast<StageId>(stage_id);
    checkpoint.rank = static_cast<int32_t>(rank);
    if ((checkpoint.stage == StageId::Extra && stage_count != 1) ||
        (!stage_ids.empty() &&
         stage_id != std::to_underlying(stage_ids.back()) + 1)) {
      return false;
    }
    stage_ids.push_back(checkpoint.stage);
    if (i == 0) {
      player_type = static_cast<PlayerType>(checkpoint.player.player_type);
    }

    ReplayStage replay_stage{.checkpoint = checkpoint};
    if (stages != nullptr) {
      const auto input_data = archive.Extract(i + 1);
      if (input_data.empty() ||
          input_data.size() != checkpoint.frame_count * sizeof(uint16_t)) {
        return false;
      }
      util::ByteReader input_reader{input_data};
      replay_stage.inputs.reserve(checkpoint.frame_count);
      for (uint32_t frame = 0; frame < checkpoint.frame_count; frame++) {
        uint16_t input = 0;
        if (!ReadValue(input_reader, input)) {
          return false;
        }
        replay_stage.inputs.push_back(static_cast<InputBits>(input));
      }
    }
    parsed_stages.push_back(std::move(replay_stage));
  }
  if (!reader.Empty()) {
    return false;
  }

  if (record != nullptr) {
    record->emplace(ReplayRecord{
        .path = {},
        .name = std::string{reinterpret_cast<const char *>(name_bytes->data()),
                            name_bytes->size()},
        .created_at = static_cast<int64_t>(created_at),
        .difficulty = static_cast<GameLevel>(difficulty),
        .player_type = *player_type,
        .stages = std::move(stage_ids),
    });
  }
  if (settings != nullptr) {
    *settings = {
        .difficulty = static_cast<GameLevel>(difficulty),
        .practice_mode = static_cast<PracticeMode>(practice_mode),
        .player_stock = player_stock,
        .bomb_stock = bomb_stock,
        .input_flags = input_flags,
    };
  }
  if (stages != nullptr) {
    *stages = std::move(parsed_stages);
  }
  return true;
}

bool RecordSystem::LoadReplay(std::string_view path, StageId start_stage) {
  ReplaySettings settings;
  std::vector<ReplayStage> stages;
  std::optional<ReplayRecord> record;
  if (!LoadArchive(path, &record, &settings, &stages)) {
    logging::Error(logging::Channel::Record,
                   "Failed to load replay archive: {}", path);
    return false;
  }
  if (!record) {
    logging::Error(logging::Channel::Record,
                   "Replay archive did not contain a record: {}", path);
    return false;
  }
  const auto selected = std::ranges::find(record->stages, start_stage);
  if (selected == record->stages.end()) {
    logging::Error(logging::Channel::Record,
                   "Replay {} does not contain stage {}", path,
                   std::to_underlying(start_stage) + 1);
    return false;
  }

  state_.emplace<PlaybackState>(PlaybackState{
      .settings = settings,
      .stages = std::move(stages),
      .stage_index =
          static_cast<std::size_t>(selected - record->stages.begin()),
      .multi_stage = true,
  });
  logging::Debug(logging::Channel::Record,
                 "Loaded replay archive {} from stage {}", path,
                 std::to_underlying(start_stage) + 1);
  return true;
}

bool RecordSystem::ConfigurePlayback(Player &player, GameSession &session) {
  auto *playback = std::get_if<PlaybackState>(&state_);
  if (playback == nullptr || playback->stages.empty() ||
      playback->stage_index >= playback->stages.size()) {
    return false;
  }
  const auto practice_mode =
      playback->settings.practice_mode == PracticeMode::AutoBomb
          ? PracticeMode::Off
          : playback->settings.practice_mode;
  player.Configure(practice_mode,
                   (playback->settings.input_flags & kInputFlagSpeedDown) != 0);
  session.level = playback->settings.difficulty;
  session.stage = CurrentPlaybackStage();
  return true;
}

void RecordSystem::RestorePlaybackStage(Player &player, GameSession &session) {
  auto &playback = std::get<PlaybackState>(state_);
  const auto &checkpoint = playback.stages[playback.stage_index].checkpoint;
  session.stage = checkpoint.stage;
  player.RestoreProgress(checkpoint.player);
  math::RestoreRandomState(checkpoint.rng);
  playback.frame_cursor = 0;
  playback.active = true;
}

bool RecordSystem::HasNextPlaybackStage() const {
  const auto *playback = std::get_if<PlaybackState>(&state_);
  return playback != nullptr && playback->multi_stage &&
         playback->stage_index + 1 < playback->stages.size();
}

bool RecordSystem::AdvancePlaybackStage() {
  if (!HasNextPlaybackStage()) {
    return false;
  }
  auto &playback = std::get<PlaybackState>(state_);
  ++playback.stage_index;
  playback.frame_cursor = 0;
  playback.active = false;
  return true;
}

StageId RecordSystem::CurrentPlaybackStage() const {
  const auto &playback = std::get<PlaybackState>(state_);
  return playback.stages[playback.stage_index].checkpoint.stage;
}

bool RecordSystem::HasStageDemo(StageId stage) const {
  const auto index = std::to_underlying(stage);
  if (index >= kRegularStageCount) {
    return false;
  }

  ReplaySettings settings;
  std::vector<ReplayStage> stages;
  std::error_code error;
  const auto local_path = DemoPath(stage);
  if (std::filesystem::is_regular_file(local_path, error) &&
      LoadDemoArchive(data::PbgArchive::Open(local_path), stage, settings,
                      stages)) {
    return true;
  }
  return index < data_.DemoCount() &&
         LoadDemoArchive(data::PbgArchive::Open(data_.ExtractDemo(index)),
                         stage, settings, stages);
}

bool RecordSystem::LoadStageDemo(StageId stage, Player &player,
                                 GameSession &session) {
  ReplaySettings settings;
  std::vector<ReplayStage> stages;
  std::error_code error;
  const auto local_path = DemoPath(stage);
  bool loaded = std::filesystem::is_regular_file(local_path, error) &&
                LoadDemoArchive(data::PbgArchive::Open(local_path), stage,
                                settings, stages);
  const auto index = std::to_underlying(stage);
  if (!loaded && index < data_.DemoCount()) {
    loaded = LoadDemoArchive(data::PbgArchive::Open(data_.ExtractDemo(index)),
                             stage, settings, stages);
  }
  if (!loaded) {
    return false;
  }

  state_.emplace<PlaybackState>(PlaybackState{
      .settings = settings,
      .stages = std::move(stages),
      .multi_stage = false,
  });
  if (!ConfigurePlayback(player, session)) {
    state_.emplace<IdleState>();
    return false;
  }
  RestorePlaybackStage(player, session);
  return true;
}

bool RecordSystem::LoadDemoArchive(const data::PbgArchive &archive,
                                   StageId stage, ReplaySettings &settings,
                                   std::vector<ReplayStage> &stages) {
  stages.clear();
  return archive && LoadArchive(archive, nullptr, &settings, &stages) &&
         stages.size() == 1 && stages.front().checkpoint.stage == stage &&
         std::ranges::any_of(stages.front().inputs, [](auto input) {
           return (input & KeyDemoStart) != 0;
         });
}

InputBits RecordSystem::NextInput() {
  auto *playback = std::get_if<PlaybackState>(&state_);
  if (playback == nullptr || !playback->active || playback->stages.empty()) {
    return KeyEscape;
  }
  const auto &inputs = playback->stages[playback->stage_index].inputs;
  if (playback->frame_cursor >= inputs.size()) {
    playback->active = false;
    return KeyEscape;
  }
  return inputs[playback->frame_cursor++];
}

void RecordSystem::StopPlayback() {
  if (std::holds_alternative<PlaybackState>(state_)) {
    state_.emplace<IdleState>();
  }
}

bool RecordSystem::IsPlaying() const {
  const auto *playback = std::get_if<PlaybackState>(&state_);
  return playback != nullptr && playback->active;
}

bool RecordSystem::IsRecording() const {
  const auto *recording = std::get_if<RecordingState>(&state_);
  return recording != nullptr &&
         (!recording->demo_capture || recording->demo_start_frame.has_value());
}

bool RecordSystem::IsMultiStagePlayback() const {
  const auto *playback = std::get_if<PlaybackState>(&state_);
  return playback != nullptr && playback->multi_stage;
}
