///
/// Score - Score input/output functions
///

// GCC 15 throws `error: conflicting declaration 'typedef struct imaxdiv_t
// imaxdiv_t'` if this appears after a module import.
#include "score.h"
#include "game/guard.h"
#include "level.h"
#include "lz_uty.h"
#include "score_manager.h"
#include <array>
#include <cinttypes> // for PRId64
#include <format>
#include <ranges>
#include <utility>

// Type aliases moved to private in score_manager.h
// ScoreData moved to Scores.score_cache
// GameFlow.score_string[] moved to Scores.score_strings

constexpr char ScoreFileName[] = "秋霜SC.DAT"; // Score data file name

// Get current score string (with name insertion)
// If NData == NULL, no insertion
uint8_t ScoreManager::SetScoreString(NrNameData *NData, uint8_t Dif) {
  NrScoreString *Res = nullptr;
  int i = 0;
  int num = 0;
  int64_t temp = 0;

  Res = score_strings.data();

  // Load score data
  uint8_t rank = 0;
  if (NData != nullptr) {
    rank = IsHighScore(NData, Dif);
    if (rank == 0) {
      return 0;
    }
  } else {
    rank = NR_RANK_MAX;
  }

  // Set the pointer
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
    // Shift scores down first
    for (i = NR_RANK_MAX - 1; std::cmp_greater_equal(i, rank); i--) {
      p[i] = p[i - 1]; // struct assignment
    }

  // Insert new data
    p[rank - 1] = *NData;
  }

  // Start data storage
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
  // Cannot load, fail
  if (!LoadScoreData()) {
    return 0;
  }
  auto score_guard = make_guard([this] { ReleaseScoreData(); });

// Assign pointer by difficulty
  const auto maybe_temp = GetNList(Dif);
  if (!maybe_temp) {
    return 0;
  }
  const auto temp = maybe_temp.value();

  // Find matching slot
  for (const auto Rank : std::views::iota(0U, temp.size())) {
    // Equal scores go to lower rank
    // (Same as previous game [Shushoku(Temporary])
    if (NData->Score > temp[Rank].Score) {
      // Return rank
      return (Rank + 1);
    }
  }
  return 0;
}

// Write score data
bool ScoreManager::SaveScoreData(NrNameData *NData, uint8_t Dif) {
  // Load score data
  const auto Rank = IsHighScore(NData, Dif);

  // Not a high score
  if (Rank == 0) {
    return false;
  }

  // Set the pointer
  if (!LoadScoreData()) {
    return false;
  }
  auto maybe_temp = GetNList(Dif);
  if (!maybe_temp) {
    ReleaseScoreData();
    return false;
  }
  auto temp = maybe_temp.value();

  // Shift scores down first
  for (auto i = (temp.size() - 1); i >= Rank; i--) {
    temp[i] = temp[i - 1]; // struct assignment
  }

  // Insert new data
  temp[Rank - 1] = *NData;

  // Write to file
  BIT_DEVICE_WRITE bd;
  SaveSC(score_cache->Easy, bd);
  SaveSC(score_cache->Normal, bd);
  SaveSC(score_cache->Hard, bd);
  SaveSC(score_cache->Lunatic, bd);
  SaveSC(score_cache->Extra, bd);
  ReleaseScoreData();

  return bd.Write(ScoreFileName);
}

// Load score data
bool ScoreManager::LoadScoreData() {
  bool bInit = false;

  // Already loaded (not a failure)
  if (score_cache) {
    return true;
  }

  score_cache = std::unique_ptr<NrScoreData>(new (std::nothrow) NrScoreData);
  if (score_cache == nullptr) {
    return false;
  }

  // Open file in bit read mode
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
    // If file does not exist or is invalid, create new one
    // No write to file at this point
    return SetDefaultScoreData();
  }

  return true;
}

void ScoreManager::ReleaseScoreData() {
  // Release
  score_cache = nullptr;
}

// Assign pointer by difficulty
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

// Set default score data
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

    // Acquire name
    for (auto &c : nd.Name) {
      c = XGet<uint8_t>(bd, Mask);
      CheckSum += c;
    }
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire score
    nd.Score = XGet<uint64_t>(bd, Mask);
    CheckSum += nd.Score;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire graze
    nd.Evade = XGet<uint32_t>(bd, Mask);
    CheckSum += nd.Evade;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire stage
    nd.Stage = XGet<uint8_t>(bd, Mask);
    CheckSum += nd.Stage;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // Acquire weapon
    nd.Weapon = XGet<uint8_t>(bd, Mask);
    CheckSum += nd.Weapon;
    if (flag != bd.GetBit()) {
      return false;
    }
    flag = 1 - flag;

    // Checksum comparison
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
    flag = 1 - flag; // Bit insertion

    // Output name
    for (const auto &c : nd.Name) {
      CheckSum += c;
      XPut(bd, static_cast<unsigned char>(c), Mask);
    }
    bd.PutBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output score
    CheckSum += nd.Score;
    XPut(bd, static_cast<uint64_t>(nd.Score), Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output graze
    CheckSum += nd.Evade;
    XPut(bd, nd.Evade, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output stage
    CheckSum += nd.Stage;
    XPut(bd, nd.Stage, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output weapon
    CheckSum += nd.Weapon;
    XPut(bd, nd.Weapon, Mask);
    bd.PutBit(flag);
    flag = 1 - flag; // Bit insertion

    // Output checksum
    XPut(bd, CheckSum, Mask);
  }
}
