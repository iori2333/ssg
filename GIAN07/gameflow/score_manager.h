///
/// ScoreManager - Centralized score data persistence and display state
///

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <span>

#include "score.h"

#include "core/level.h"
#include "core/lz_uty.h"

struct ScoreManager {
  // Score display strings
  std::array<NrScoreString, NR_RANK_MAX> score_strings = {};

  // Score data cache (formerly file-static in SCORE.cpp)
  std::unique_ptr<NrScoreData> score_cache = nullptr;

  // Mask constant
  static constexpr uint64_t PBG_MASK_VALUE = 0xb97eb2c6542d3a41;

  // === Public methods ===

  // Get current score string (Ret: 0=not high score, otherwise=rank)
  [[nodiscard]] uint8_t SetScoreString(NrNameData *NData, GameLevel Dif);

  // Check if high score (0: not high score, otherwise: rank)
  [[nodiscard]] uint8_t IsHighScore(const NrNameData *NData, GameLevel Dif);

  // Save score data
  [[nodiscard]] bool SaveScoreData(NrNameData *NData, GameLevel Dif);

private:
  // Type aliases
  using NR_SCORE_LIST = std::span<NrNameData, NR_RANK_MAX>;
  using NR_CONST_SCORE_LIST = std::span<const NrNameData, NR_RANK_MAX>;

  // Internal helpers
  bool LoadScoreData();
  void ReleaseScoreData();
  std::optional<NR_SCORE_LIST> GetNList(GameLevel Dif) const;
  bool SetDefaultScoreData();

  bool LoadSC(NR_SCORE_LIST NData, BIT_DEVICE_READ &bd);
  void SaveSC(NR_CONST_SCORE_LIST NData, BIT_DEVICE_WRITE &bd);

  template <std::unsigned_integral T>
  T XGet(BIT_DEVICE_READ &bd, uint64_t &ExMask);

  template <std::unsigned_integral T>
  void XPut(BIT_DEVICE_WRITE &bd, T data, uint64_t &ExMask);
};

template <std::unsigned_integral T>
T ScoreManager::XGet(BIT_DEVICE_READ &bd, uint64_t &ExMask) {
  ExMask = (((ExMask & 0x800000000000000) >> 60) + (ExMask << 1));

  T ret = 0;
  if constexpr (sizeof(T) == 1) {
    ret = bd.GetBits(8);
  } else {
    for (const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
      ret |= (static_cast<T>(bd.GetBits(16)) << (word * 16));
    }
  }
  return (ret - static_cast<T>(ExMask));
}

template <std::unsigned_integral T>
void ScoreManager::XPut(BIT_DEVICE_WRITE &bd, T data, uint64_t &ExMask) {
  ExMask = (((ExMask & 0x800000000000000) >> 60) + (ExMask << 1));

  const auto temp = (data + static_cast<T>(ExMask));

  if constexpr (sizeof(T) == 1) {
    bd.PutBits(temp, 8);
  } else {
    for (const auto word : std::views::iota(0u, (sizeof(T) / 2))) {
      bd.PutBits((temp >> (word * 16)), 16);
    }
  }
}
