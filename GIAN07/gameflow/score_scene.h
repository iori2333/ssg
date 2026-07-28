/// Score leaderboard, name registration, and score persistence.

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>

#include "gameplay/game_rules.h"
#include "sys/bit_stream.h"
#include "ui/name_entry.h"

inline constexpr std::size_t kScoreNameLength = 9;
inline constexpr std::size_t kScoreRankCount = 5;

struct ScoreEntry {
  char name[kScoreNameLength];
  int64_t score;
  uint32_t graze;
  uint8_t stage;
  uint8_t weapon;
};

struct ScoreData {
  std::array<ScoreEntry, kScoreRankCount> easy;
  std::array<ScoreEntry, kScoreRankCount> normal;
  std::array<ScoreEntry, kScoreRankCount> hard;
  std::array<ScoreEntry, kScoreRankCount> lunatic;
  std::array<ScoreEntry, kScoreRankCount> extra;
};

struct ScoreRow {
  uint8_t rank;
  int x;
  int y;
  bool moving;
  char name[kScoreNameLength];
  std::string score;
  std::string graze;
  std::string stage;
  uint8_t weapon;
};

class ScoreScene {
public:
  [[nodiscard]] bool ShowLeaderboard();
  [[nodiscard]] bool StartNameRegistration(bool change_music = false);
  void UpdateLeaderboard(bool &);
  void UpdateNameRegistration(bool &);

private:
  using ScoreList = std::span<ScoreEntry, kScoreRankCount>;
  using ConstScoreList = std::span<const ScoreEntry, kScoreRankCount>;

  static constexpr uint64_t kScoreMask = 0xb97eb2c6542d3a41;

  [[nodiscard]] GameLevel CurrentLevel() const;
  void DrawScores();

  [[nodiscard]] uint8_t BuildRows(ScoreEntry *entry, GameLevel difficulty);
  [[nodiscard]] uint8_t RankOf(const ScoreEntry &entry, GameLevel difficulty);
  [[nodiscard]] bool Save(const ScoreEntry &entry, GameLevel difficulty);
  [[nodiscard]] bool Load();
  void Release();
  [[nodiscard]] std::optional<ScoreList> ScoresFor(GameLevel difficulty) const;
  [[nodiscard]] bool SetDefaults();
  [[nodiscard]] bool LoadList(ScoreList entries, BitReader &reader);
  void SaveList(ConstScoreList entries, BitWriter &writer);

  template <std::unsigned_integral T>
  static T ReadMasked(BitReader &reader, uint64_t &mask);

  template <std::unsigned_integral T>
  static void WriteMasked(BitWriter &writer, T data, uint64_t &mask);

  std::array<ScoreRow, kScoreRankCount> rows_{};
  std::unique_ptr<ScoreData> score_cache_;
  ScoreEntry current_entry_{};
  uint8_t current_rank_ = 0;
  uint8_t current_difficulty_ = 0;
  bool input_locked_ = false;
  NameEntry name_entry_;
};

template <std::unsigned_integral T>
T ScoreScene::ReadMasked(BitReader &reader, uint64_t &mask) {
  mask = (((mask & 0x800000000000000) >> 60) + (mask << 1));

  T result = 0;
  if constexpr (sizeof(T) == 1) {
    result = reader.ReadBits(8);
  } else {
    for (const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
      result |= (static_cast<T>(reader.ReadBits(16)) << (word * 16));
    }
  }
  return result - static_cast<T>(mask);
}

template <std::unsigned_integral T>
void ScoreScene::WriteMasked(BitWriter &writer, T data, uint64_t &mask) {
  mask = (((mask & 0x800000000000000) >> 60) + (mask << 1));
  const auto encoded = data + static_cast<T>(mask);

  if constexpr (sizeof(T) == 1) {
    writer.WriteBits(encoded, 8);
  } else {
    for (const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
      writer.WriteBits((encoded >> (word * 16)), 16);
    }
  }
}
