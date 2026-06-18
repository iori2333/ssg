/*                                                                           */
/*   Score.cpp   スコア入出力関数                                            */
/*                                                                           */
/*                                                                           */

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include "score.h"
#include "game/defer.h"
#include "level.h"
#include "lz_uty.h"
#include "score_manager.h"
#include <array>
#include <cinttypes> // for PRId64
#include <format>
#include <ranges>
#include <utility>

// 型エイリアス → score_manager.h の private に移動
// ScoreData → Scores.score_cache に移動
// GameFlow.score_string[] → Scores.score_strings に移動

constexpr char ScoreFileName[] = "秋霜SC.DAT"; // スコアデータ格納ファイル名

// 現在のスコア列を取得する(名前挿入アリ) //
// NData == NULL の場合、挿入しません     //
uint8_t ScoreManager::SetScoreString(NrNameData *NData, uint8_t Dif) {
  NrScoreString *Res = nullptr;
  int i = 0;
  int num = 0;
  int64_t temp = 0;

  Res = score_strings.data();

  // スコアデータを読み込む //
  uint8_t rank = 0;
  if (NData != nullptr) {
    rank = IsHighScore(NData, Dif);
    if (rank == 0) {
      return 0;
    }
  } else {
    rank = NR_RANK_MAX;
  }

  // ポインタをセットするぞ //
  if (!LoadScoreData()) {
    return 0;
  }
  auto maybe_p = GetNList(Dif);
  if (!maybe_p) {
    ReleaseScoreData();
    return 0;
  }
  auto p = maybe_p.value();

  if ((rank != 0) && (NData != nullptr)) {
    // まずは、スコアを下方向に押し出すのだ //
    for (i = NR_RANK_MAX - 1; std::cmp_greater_equal(i, rank); i--) {
      p[i] = p[i - 1]; // 構造体から構造体への代入
    }

    // 新しいデータを挿入だ //
    p[rank - 1] = *NData;
  }

  // データの格納開始 //
  temp = num = 0;
  for (i = 0; std::cmp_less(i, NR_RANK_MAX); i++) {
    if (temp < p[i].Score) {
      temp = p[i].Score;
      num += 1;
    }

    Res[i].Rank = num;
    Res[i].x = (640 + (50 + (i * 24 * 20))) << 6;
    Res[i].y = (100 + (i * 48)) << 6;
    Res[i].bMoveEnable = true;

    std::format_to_n(Res[i].Name, NR_NAME_LEN, "{}", p[i].Name);

    Res[i].Weapon = p[i].Weapon % 4;

    Res[i].Score = std::format("{:11}", p[i].Score);
    Res[i].Evade = std::format("{:6}", p[i].Evade);
    Res[i].Stage = std::format("{:1}", p[i].Stage);
  }

  ReleaseScoreData();

  return rank;
}

uint8_t ScoreManager::IsHighScore(const NrNameData *NData, uint8_t Dif) {
  // ロードできないので失敗！ //
  if (!LoadScoreData()) {
    return 0;
  }
  defer(ReleaseScoreData());

  // 難易度でポインタを振り分ける //
  const auto maybe_temp = GetNList(Dif);
  if (!maybe_temp) {
    return 0;
  }
  const auto temp = maybe_temp.value();

  // 該当個所はあるかな？ //
  for (const auto Rank : std::views::iota(0U, temp.size())) {
    // スコアが等しい場合は、後から入ったほうが下の順位に //
    // なるようにするのだ（前作[秋霜玉(仮] と同じね）     //
    if (NData->Score > temp[Rank].Score) {
      // 順位を返す //
      return (Rank + 1);
    }
  }
  return 0;
}

// スコアデータを書き出す //
bool ScoreManager::SaveScoreData(NrNameData *NData, uint8_t Dif) {
  // スコアデータを読み込む //
  const auto Rank = IsHighScore(NData, Dif);

  // これは、ハイスコアじゃないね //
  if (Rank == 0) {
    return false;
  }

  // ポインタをセットする //
  if (!LoadScoreData()) {
    return false;
  }
  auto maybe_temp = GetNList(Dif);
  if (!maybe_temp) {
    ReleaseScoreData();
    return false;
  }
  auto temp = maybe_temp.value();

  // まずは、スコアを下方向に押し出す //
  for (auto i = (temp.size() - 1); i >= Rank; i--) {
    temp[i] = temp[i - 1]; // 構造体から構造体への代入
  }

  // 新しいデータを挿入だ //
  temp[Rank - 1] = *NData;

  // 実際にファイルに出力 //
  BIT_DEVICE_WRITE bd;
  SaveSC(score_cache->Easy, bd);
  SaveSC(score_cache->Normal, bd);
  SaveSC(score_cache->Hard, bd);
  SaveSC(score_cache->Lunatic, bd);
  SaveSC(score_cache->Extra, bd);
  ReleaseScoreData();

  return bd.Write(ScoreFileName);
}

// スコアデータを読み込む //
bool ScoreManager::LoadScoreData() {
  bool bInit = false;

  // すでにロード済みの場合(これは失敗にしない) //
  if (score_cache) {
    return true;
  }

  score_cache = std::unique_ptr<NrScoreData>(new (std::nothrow) NrScoreData);
  if (score_cache == nullptr) {
    return false;
  }

  // ビット読み込みモードでファイルを開く //
  auto bd = BitFilCreateR(ScoreFileName);
  while (1) {
    if (!LoadSC(score_cache->Easy, bd)) {
      break;
    }
    if (!LoadSC(score_cache->Normal, bd)) {
      break;
    }
    if (!LoadSC(score_cache->Hard, bd)) {
      break;
    }
    if (!LoadSC(score_cache->Lunatic, bd)) {
      break;
    }
    if (!LoadSC(score_cache->Extra, bd)) {
      break;
    }

    bInit = true;
    break;
  }

  if (!bInit) {
    // ファイルが存在しないか不正な場合、新たに作成する //
    // この時点では、ファイルに対して書き込みは行わない //
    return SetDefaultScoreData();
  }

  return true;
}

void ScoreManager::ReleaseScoreData() {
  // 解放～ //
  score_cache = nullptr;
}

// 難易度でポインタを振り分ける //
std::optional<ScoreManager::NR_SCORE_LIST>
ScoreManager::GetNList(uint8_t Dif) const {
  if (!score_cache) {
    return {};
  }

  switch (Dif) {
  case GAME_EASY:
    return score_cache->Easy;
  case GAME_NORMAL:
    return score_cache->Normal;
  case GAME_HARD:
    return score_cache->Hard;
  case GAME_LUNATIC:
    return score_cache->Lunatic;
  case GAME_EXTRA:
    return score_cache->Extra;
  default:
    return {};
  }
}

// スコアデータ初期値をセット //
bool ScoreManager::SetDefaultScoreData() {
  if (nullptr == score_cache) {
    return false;
  }

  for (auto i = 0; i < (GAME_EXTRA + 1); i++) {
    auto maybe_temp = GetNList(i);
    if (!maybe_temp) {
      return false;
    }
    auto temp = maybe_temp.value();
    for (size_t j = 0; j < temp.size(); j++) {
      std::format_to_n(temp[j].Name, NR_NAME_LEN, "????????");
      temp[j].Score = ((temp.size() - j) * uint64_t{1200000});
      temp[j].Evade = ((temp.size() - j) * 50);
      temp[j].Stage = ((i < 4) ? (temp.size() - j) : 1);
      temp[j].Weapon = j % 3;
    }
  }

  return true;
}

bool ScoreManager::LoadSC(NR_SCORE_LIST NData, BIT_DEVICE_READ &bd) {
  uint64_t CheckSum = 0;
  uint64_t Mask = PBG_MASK_VALUE;
  uint8_t flag = 0;

  for (auto &nd : NData) {
    CheckSum = 0;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // 名前を獲得する //
    for (auto &c : nd.Name) {
      c = XGet<uint8_t>(bd, Mask);
      CheckSum += c;
    }
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // 得点を獲得する //
    nd.Score = XGet<uint64_t>(bd, Mask);
    CheckSum += nd.Score;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // かすりを獲得する //
    nd.Evade = XGet<uint32_t>(bd, Mask);
    CheckSum += nd.Evade;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // ステージを獲得する //
    nd.Stage = XGet<uint8_t>(bd, Mask);
    CheckSum += nd.Stage;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // ウエポンを獲得する //
    nd.Weapon = XGet<uint8_t>(bd, Mask);
    CheckSum += nd.Weapon;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // チェックサム比較 //
    if (CheckSum != XGet<uint64_t>(bd, Mask)) {
      return false;
    }
  }

  return true;
}

void ScoreManager::SaveSC(NR_CONST_SCORE_LIST NData, BIT_DEVICE_WRITE &bd) {
  uint64_t CheckSum = 0;
  uint64_t Mask = PBG_MASK_VALUE;
  uint8_t flag = 0;

  for (const auto &nd : NData) {
    CheckSum = 0;
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // 名前を出力する //
    for (const auto &c : nd.Name) {
      CheckSum += c;
      XPut(bd, static_cast<unsigned char>(c), Mask);
    }
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // 得点を出力する //
    CheckSum += nd.Score;
    XPut(bd, static_cast<uint64_t>(nd.Score), Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // かすりを出力する //
    CheckSum += nd.Evade;
    XPut(bd, nd.Evade, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // ステージを出力する //
    CheckSum += nd.Stage;
    XPut(bd, nd.Stage, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // ウエポンを出力する //
    CheckSum += nd.Weapon;
    XPut(bd, nd.Weapon, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // ビット挿入

    // チェックサムを出力する //
    XPut(bd, CheckSum, Mask);
  }
}
