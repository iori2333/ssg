/*
 *   ScoreManager — centralized score data persistence and display state
 */

#pragma once

#include "lz_uty.h"
#include "score.h"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>
#include <span>

struct ScoreManager {
  // スコア表示用文字列
  std::array<NrScoreString, NR_RANK_MAX> score_strings = {};

  // スコアデータキャッシュ（旧 SCORE.cpp ファイル静的変数）
  std::unique_ptr<NrScoreData> score_cache = nullptr;

  // マスク定数
  static constexpr uint64_t PBG_MASK_VALUE = 0xb97eb2c6542d3a41;

  // === 公開メソッド ===

  // 現在のスコア列を取得する（Ret: 0=ハイスコアでない それ以外=順位）
  [[nodiscard]] uint8_t SetScoreString(NrNameData *NData, uint8_t Dif);

  // ハイスコアかどうか（0: ハイスコアでない, それ以外: 順位）
  [[nodiscard]] uint8_t IsHighScore(const NrNameData *NData, uint8_t Dif);

  // スコアデータを書き出す
  [[nodiscard]] bool SaveScoreData(NrNameData *NData, uint8_t Dif);

private:
  // 型エイリアス
  using NR_SCORE_LIST = std::span<NrNameData, NR_RANK_MAX>;
  using NR_CONST_SCORE_LIST = std::span<const NrNameData, NR_RANK_MAX>;

  // 内部ヘルパー
  bool LoadScoreData();
  void ReleaseScoreData();
  std::optional<NR_SCORE_LIST> GetNList(uint8_t Dif);
  bool SetDefaultScoreData();

  bool LoadSC(NR_SCORE_LIST NData, BIT_DEVICE_READ &bd);
  void SaveSC(NR_CONST_SCORE_LIST NData, BIT_DEVICE_WRITE &bd);

  template <std::unsigned_integral T>
  T XGet(BIT_DEVICE_READ &bd, uint64_t &ExMask);

  template <std::unsigned_integral T>
  void XPut(BIT_DEVICE_WRITE &bd, T data, uint64_t &ExMask);
};

extern ScoreManager Scores;

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
