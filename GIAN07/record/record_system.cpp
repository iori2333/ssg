/// Game record persistence, Replay recording, and playback.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
#include <limits>
#include <optional>
#include <ranges>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include <SDL3/SDL_iostream.h>

#include "record_system.h"

#include "data/pbg_archive.h"
#include "gameflow/gameflow_manager.h"
#include "gameplay/game_session.h"
#include "settings/config.h"
#include "sys/file.h"
#include "util/byte_io.h"
#include "util/endian.h"
#include "util/ut_math.h"

namespace {

constexpr std::array<uint8_t, 4> kReplayMagic = {'G', '7', 'R', 'P'};
constexpr uint16_t kReplayVersion = 2;
constexpr auto kReplayDirectory = "replays";
constexpr std::array<uint8_t, 4> kScoreMagic = {'G', '7', 'S', 'C'};
constexpr uint16_t kScoreVersion = 1;
constexpr auto kScoreDirectory = "scores";
constexpr auto kMaxSignedRecordValue =
    static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

struct ScoreRecordFile {
  std::array<uint8_t, 4> magic{};
  ENDIAN_LITTLE<uint16_t> version = 0;
  ENDIAN_LITTLE<uint64_t> created_at = 0;
  ENDIAN_LITTLE<uint64_t> score = 0;
  ENDIAN_LITTLE<uint32_t> graze = 0;
  ENDIAN_LITTLE<uint16_t> miss_count = 0;
  ENDIAN_LITTLE<uint16_t> bomb_used = 0;
  ENDIAN_LITTLE<uint16_t> deathbomb_count = 0;
  uint8_t difficulty = 0;
  uint8_t stage = 0;
  uint8_t player_type = 0;
  uint8_t name_length = 0;
  std::array<char, kRecordNameLength> name{};
};
static_assert(sizeof(ScoreRecordFile) == 44);

struct DemoReplayConfig {
  uint8_t game_level;
  uint8_t player_stock;
  uint8_t bomb_stock;
  uint8_t padding_1[5];
  uint8_t input_flags;
  uint8_t padding_2[15];
};
static_assert(sizeof(DemoReplayConfig) == 24);

struct DemoReplayHeader {
  uint32_t random_seed;
  uint32_t frame_count;
  DemoReplayConfig config;
  uint8_t power;
  uint8_t weapon;
};
static_assert(sizeof(DemoReplayHeader) == 36);

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

bool IsReplayFilename(std::string_view name) {
  const auto replay_prefix = name.starts_with("replay_");
  const auto extra_prefix = name.starts_with("replay_ex_");
  const auto extension = std::filesystem::path{name}.extension().string();
  return (replay_prefix || extra_prefix) && extension == ".dat";
}

bool IsScoreFilename(const std::filesystem::path &path) {
  const auto filename = path.filename().string();
  return filename.starts_with("score_") && path.extension().string() == ".dat";
}

std::optional<ScoreRecord> LoadScoreRecord(const std::filesystem::path &path) {
  const auto bytes = SDL_LoadFile(path.string().c_str());
  if (bytes.size() != sizeof(ScoreRecordFile)) {
    return std::nullopt;
  }
  auto cursor = bytes.cursor();
  const auto file = cursor.next<ScoreRecordFile>();
  if (!file || file->front().magic != kScoreMagic ||
      file->front().version != kScoreVersion ||
      file->front().created_at > kMaxSignedRecordValue ||
      file->front().score > kMaxSignedRecordValue ||
      file->front().difficulty > std::to_underlying(GameLevel::Extra) ||
      file->front().stage > std::to_underlying(StageId::Extra) ||
      file->front().player_type > std::to_underlying(PlayerType::Laser) ||
      file->front().name_length > kRecordNameLength) {
    return std::nullopt;
  }

  const auto &data = file->front();
  return ScoreRecord{
      .name = std::string{data.name.data(), data.name_length},
      .created_at = static_cast<int64_t>(data.created_at),
      .score = static_cast<int64_t>(data.score),
      .graze = data.graze,
      .miss_count = data.miss_count,
      .bomb_used = data.bomb_used,
      .deathbomb_count = data.deathbomb_count,
      .difficulty = static_cast<GameLevel>(data.difficulty),
      .stage = static_cast<StageId>(data.stage),
      .player_type = static_cast<PlayerType>(data.player_type),
  };
}

} // namespace

ScoreRecord RecordSystem::CaptureScore(const Player &player,
                                       const GameSession &session) const {
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
                                                  std::size_t limit) const {
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

bool RecordSystem::SaveScore(const ScoreRecord &record) const {
  std::error_code error;
  std::filesystem::create_directories(kScoreDirectory, error);
  if (error) {
    return false;
  }

  const auto now = std::chrono::system_clock::now();
  const auto file_id = std::chrono::duration_cast<std::chrono::milliseconds>(
                           now.time_since_epoch())
                           .count();
  auto path = std::filesystem::path{kScoreDirectory} /
              std::format("score_{}.dat", file_id);
  for (uint32_t suffix = 1; std::filesystem::exists(path, error); suffix++) {
    if (error) {
      return false;
    }
    path = std::filesystem::path{kScoreDirectory} /
           std::format("score_{}_{}.dat", file_id, suffix);
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

  auto *stream = SDL_IOFromFile(path.string().c_str(), "wb");
  if (stream == nullptr) {
    return false;
  }
  const bool written = SDL_MustWriteIO(stream, &file, sizeof(file));
  return SDL_CloseIO(stream) && written;
}

void RecordSystem::BeginRecording(const Player &player,
                                  const GameSession &session,
                                  const ConfigData &config) {
  playing_ = false;
  multi_stage_playback_ = false;
  playback_stages_.clear();

  const auto seed = ((static_cast<uint32_t>(rnd()) + 1U) *
                     (static_cast<uint32_t>(rnd()) + 1U));
  rnd_seed_set(seed);

  settings_ = {
      .difficulty = session.level,
      .practice_mode = config.game.practice_mode,
      .player_stock = config.game.player_stock,
      .bomb_stock = config.game.bomb_stock,
      .input_flags = config.input.PackFlags(),
  };
  frame_cursor_ = 0;
  recording_ = true;
  recorded_stages_.clear();
  current_inputs_.clear();
  BeginStage(player, session);
}

void RecordSystem::BeginStage(const Player &player,
                              const GameSession &session) {
  if (!recording_) {
    return;
  }
  current_checkpoint_ = {
      .stage = session.stage,
      .frame_count = 0,
      .rng = rnd_state(),
      .player = player.CaptureProgress(),
      .rank = session.rank,
  };
  has_current_checkpoint_ = true;
  current_inputs_.clear();
  frame_cursor_ = 0;
}

bool RecordSystem::HasRecordedStages() const {
  return recording_ && (!recorded_stages_.empty() || !current_inputs_.empty());
}

void RecordSystem::FlushStage() {
  if (!recording_ || !has_current_checkpoint_ || current_inputs_.empty()) {
    return;
  }
  if (recorded_stages_.size() >= kReplayStageCapacity) {
    recording_ = false;
    return;
  }

  current_checkpoint_.frame_count =
      static_cast<uint32_t>(current_inputs_.size());
  recorded_stages_.push_back({.checkpoint = current_checkpoint_,
                              .inputs = std::move(current_inputs_)});
  current_inputs_.clear();
  has_current_checkpoint_ = false;
  frame_cursor_ = 0;
}

void RecordSystem::Record(INPUT_BITS input) {
  if (!recording_ || (input & KEY_ESC) != 0 ||
      current_inputs_.size() >= kReplayBufferCapacity) {
    return;
  }
  current_inputs_.push_back(input);
}

void RecordSystem::UpdateLastRecordedInput(INPUT_BITS input) {
  if (recording_ && !current_inputs_.empty()) {
    current_inputs_.back() = input;
  }
}

void RecordSystem::CancelRecording() {
  recording_ = false;
  has_current_checkpoint_ = false;
  current_inputs_.clear();
  recorded_stages_.clear();
}

bool RecordSystem::SaveReplay(std::string_view name, bool extra_stage) {
  if (!recording_) {
    return false;
  }
  FlushStage();
  recording_ = false;
  if (recorded_stages_.empty()) {
    return false;
  }

  std::string replay_name{name.substr(0, kRecordNameLength)};
  if (replay_name.empty()) {
    replay_name = "Vivit!";
  }

  const auto now = std::chrono::system_clock::now();
  const auto created_at =
      std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
          .count();

  util::ByteWriter manifest;
  manifest.WriteBytes(kReplayMagic);
  manifest.Write(kReplayVersion);
  manifest.Write<uint64_t>(static_cast<uint64_t>(created_at));
  manifest.Write(std::to_underlying(settings_.difficulty));
  manifest.Write(std::to_underlying(settings_.practice_mode));
  manifest.Write(settings_.player_stock);
  manifest.Write(settings_.bomb_stock);
  manifest.Write(settings_.input_flags);
  manifest.Write(static_cast<uint8_t>(replay_name.size()));
  manifest.WriteBytes({reinterpret_cast<const uint8_t *>(replay_name.data()),
                       replay_name.size()});
  manifest.Write(static_cast<uint8_t>(recorded_stages_.size()));

  for (const auto &stage : recorded_stages_) {
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
  for (const auto &stage : recorded_stages_) {
    util::ByteWriter inputs;
    for (const auto input : stage.inputs) {
      inputs.Write(static_cast<uint16_t>(input));
    }
    archive.Add(inputs.Bytes());
  }

  std::error_code error;
  std::filesystem::create_directories(kReplayDirectory, error);
  const auto replay_path = std::filesystem::path{kReplayDirectory} /
                           ReplayFilename(extra_stage, now);
  const auto saved = !error && archive.Write(replay_path.string().c_str());
  recorded_stages_.clear();
  current_inputs_.clear();
  has_current_checkpoint_ = false;
  return saved;
}

std::vector<ReplayRecord> RecordSystem::ListReplays() const {
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
                               std::vector<ReplayStage> *stages) const {
  const auto archive = data::PbgArchive::Open(std::string{path}.c_str());
  if (!archive) {
    return false;
  }
  const auto manifest_data = archive.Extract(0);
  if (!manifest_data) {
    return false;
  }
  util::ByteReader reader{{manifest_data.get(), manifest_data.size()}};
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
      !ReadValue(reader, created_at) || !ReadValue(reader, difficulty) ||
      !ReadValue(reader, practice_mode) || !ReadValue(reader, player_stock) ||
      !ReadValue(reader, bomb_stock) || !ReadValue(reader, input_flags) ||
      !ReadValue(reader, name_length) || name_length > kRecordNameLength) {
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
      if (!input_data ||
          input_data.size() != checkpoint.frame_count * sizeof(uint16_t)) {
        return false;
      }
      util::ByteReader input_reader{{input_data.get(), input_data.size()}};
      replay_stage.inputs.reserve(checkpoint.frame_count);
      for (uint32_t frame = 0; frame < checkpoint.frame_count; frame++) {
        uint16_t input = 0;
        if (!ReadValue(input_reader, input)) {
          return false;
        }
        replay_stage.inputs.push_back(static_cast<INPUT_BITS>(input));
      }
    }
    parsed_stages.push_back(std::move(replay_stage));
  }
  if (!reader.Empty()) {
    return false;
  }

  if (record != nullptr) {
    record->emplace(ReplayRecord{
        .path = std::string{path},
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
  playing_ = false;
  multi_stage_playback_ = false;
  playback_stages_.clear();

  std::optional<ReplayRecord> record;
  if (!LoadArchive(path, &record, &settings_, &playback_stages_)) {
    return false;
  }
  const auto selected = std::ranges::find(record->stages, start_stage);
  if (selected == record->stages.end()) {
    playback_stages_.clear();
    return false;
  }

  playback_stage_index_ =
      static_cast<std::size_t>(selected - record->stages.begin());
  frame_cursor_ = 0;
  return true;
}

bool RecordSystem::ConfigurePlayback(ConfigData &config, GameSession &session) {
  if (playback_stages_.empty() ||
      playback_stage_index_ >= playback_stages_.size()) {
    return false;
  }
  saved_config_ = {
      .difficulty = config.game.game_level,
      .practice_mode = config.game.practice_mode,
      .player_stock = config.game.player_stock,
      .bomb_stock = config.game.bomb_stock,
      .input_flags = config.input.PackFlags(),
  };
  config_overridden_ = true;

  config.game.game_level = settings_.difficulty;
  config.game.practice_mode = settings_.practice_mode == PracticeMode::AutoBomb
                                  ? PracticeMode::Off
                                  : settings_.practice_mode;
  config.game.player_stock = settings_.player_stock;
  config.game.bomb_stock = settings_.bomb_stock;
  config.input.UnpackFlags(settings_.input_flags);
  session.level = settings_.difficulty;
  session.stage = CurrentPlaybackStage();
  multi_stage_playback_ = true;
  return true;
}

void RecordSystem::RestorePlaybackStage(Player &player, GameSession &session) {
  const auto &checkpoint = playback_stages_[playback_stage_index_].checkpoint;
  session.stage = checkpoint.stage;
  session.rank = checkpoint.rank;
  player.RestoreProgress(checkpoint.player);
  rnd_state_restore(checkpoint.rng);
  frame_cursor_ = 0;
  playing_ = true;
}

bool RecordSystem::HasNextPlaybackStage() const {
  return multi_stage_playback_ &&
         playback_stage_index_ + 1 < playback_stages_.size();
}

bool RecordSystem::AdvancePlaybackStage() {
  if (!HasNextPlaybackStage()) {
    return false;
  }
  playback_stage_index_++;
  frame_cursor_ = 0;
  playing_ = false;
  return true;
}

StageId RecordSystem::CurrentPlaybackStage() const {
  return playback_stages_[playback_stage_index_].checkpoint.stage;
}

bool RecordSystem::LoadStageDemo(StageId stage) {
  playing_ = false;
  multi_stage_playback_ = false;
  playback_stages_.clear();

  const auto data = data_.ExtractMap(std::to_underlying(stage) + 6);
  auto cursor = data.cursor();
  const auto header_data = cursor.next<DemoReplayHeader>();
  if (!header_data) {
    return false;
  }
  const auto &header = header_data.value()[0];
  if (header.frame_count > kReplayBufferCapacity) {
    return false;
  }
  const auto inputs = cursor.next<uint16_t>(header.frame_count);
  if (!inputs) {
    return false;
  }

  settings_ = {
      .difficulty = static_cast<GameLevel>(header.config.game_level),
      .practice_mode = PracticeMode::Off,
      .player_stock = header.config.player_stock,
      .bomb_stock = header.config.bomb_stock,
      .input_flags = header.config.input_flags,
  };
  auto progress = GameFlow.ctx.player.CaptureProgress();
  progress.player_type = header.weapon;
  progress.power = header.power;
  progress.lives = header.config.player_stock;
  progress.bombs = header.config.bomb_stock;

  ReplayStage replay_stage;
  replay_stage.checkpoint = {
      .stage = stage,
      .frame_count = header.frame_count,
      .rng = {.seed = header.random_seed, .draw_count = 0},
      .player = progress,
      .rank = GameFlow.ctx.session.rank,
  };
  replay_stage.inputs.assign(inputs->begin(), inputs->end());
  playback_stages_.push_back(std::move(replay_stage));
  playback_stage_index_ = 0;

  saved_config_ = {
      .difficulty = GameFlow.ctx.config.game.game_level,
      .practice_mode = GameFlow.ctx.config.game.practice_mode,
      .player_stock = GameFlow.ctx.config.game.player_stock,
      .bomb_stock = GameFlow.ctx.config.game.bomb_stock,
      .input_flags = GameFlow.ctx.config.input.PackFlags(),
  };
  config_overridden_ = true;
  GameFlow.ctx.config.game.practice_mode = PracticeMode::Off;
  GameFlow.ctx.config.game.player_stock = settings_.player_stock;
  GameFlow.ctx.config.game.bomb_stock = settings_.bomb_stock;
  GameFlow.ctx.config.input.UnpackFlags(settings_.input_flags);
  GameFlow.ctx.session.level = settings_.difficulty;
  GameFlow.ctx.player.Configure(PracticeMode::Off,
                                GameFlow.ctx.config.input.z_spd_down_enabled);
  GameFlow.ctx.player.RestoreProgress(progress);
  rnd_seed_set(header.random_seed);
  playing_ = true;
  return true;
}

INPUT_BITS RecordSystem::NextInput() {
  if (!playing_ || playback_stages_.empty()) {
    return KEY_ESC;
  }
  const auto &inputs = playback_stages_[playback_stage_index_].inputs;
  if (frame_cursor_ >= inputs.size()) {
    playing_ = false;
    return KEY_ESC;
  }
  return inputs[frame_cursor_++];
}

void RecordSystem::StopPlayback(ConfigData &config, GameSession &session) {
  if (config_overridden_) {
    config.game.game_level = saved_config_.difficulty;
    config.game.practice_mode = saved_config_.practice_mode;
    config.game.player_stock = saved_config_.player_stock;
    config.game.bomb_stock = saved_config_.bomb_stock;
    config.input.UnpackFlags(saved_config_.input_flags);
    session.level = saved_config_.difficulty;
    config_overridden_ = false;
  }
  playing_ = false;
  multi_stage_playback_ = false;
  playback_stages_.clear();
  playback_stage_index_ = 0;
  frame_cursor_ = 0;
}
