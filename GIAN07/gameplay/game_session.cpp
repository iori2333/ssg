///
/// GameSession - rank and session state
///

#include <algorithm>
#include <string_view>
#include <utility>

#include "game_rules.h"
#include "game_session.h"

namespace {

constexpr auto kRankEasyDefault = 20 * 256;
constexpr auto kRankEasyMin = 16 * 256;
constexpr auto kRankEasyMax = 32 * 256;

constexpr auto kRankNormalDefault = 24 * 256;
constexpr auto kRankNormalMin = 22 * 256;
constexpr auto kRankNormalMax = 40 * 256;

constexpr auto kRankHardDefault = 36 * 256;
constexpr auto kRankHardMin = 34 * 256;
constexpr auto kRankHardMax = 52 * 256;

constexpr auto kRankLunaticDefault = 48 * 256;
constexpr auto kRankLunaticMin = 44 * 256;
constexpr auto kRankLunaticMax = 64 * 256;

} // namespace

std::string_view GameSession::DifficultyName() const {
  switch (level) {
  case GameLevel::Easy:
    return "Easy";
  case GameLevel::Normal:
    return "Normal";
  case GameLevel::Hard:
    return "Hard";
  case GameLevel::Extra:
    return "Extra";
  case GameLevel::Lunatic:
    return "Lunatic";
  }
  return "Unknown";
}

GameLevel GameSession::EffectiveLevel() const {
  switch (level) {
  case GameLevel::Easy:
    if (rank <= kRankEasyMax) {
      return GameLevel::Easy;
    }
    return GameLevel::Normal;
  case GameLevel::Normal:
    if (rank < kRankNormalMin) {
      return GameLevel::Easy;
    }
    if (rank <= kRankNormalMax) {
      return GameLevel::Normal;
    }
    return GameLevel::Hard;
  case GameLevel::Hard:
  case GameLevel::Extra:
    if (rank < kRankHardMin) {
      return GameLevel::Normal;
    }
    if (rank <= kRankHardMax) {
      return GameLevel::Hard;
    }
    return GameLevel::Lunatic;
  case GameLevel::Lunatic:
    if (rank < kRankLunaticMin) {
      return GameLevel::Hard;
    }
    return GameLevel::Lunatic;
  }
  return GameLevel::Normal;
}

void GameSession::AdvanceStage() {
  if (stage < StageId::Stage6) {
    stage = static_cast<StageId>(std::to_underlying(stage) + 1);
  }
}

void GameSession::UpdateRank(uint32_t stage_frame) {
  if (stage_frame % 60 == 0) {
    AddRank(1);
  }
}

void GameSession::AddRank(int n) {
  rank += n;

  switch (level) {
  case GameLevel::Easy:
    rank = std::clamp(rank, kRankEasyMin, kRankEasyMax);
    break;
  case GameLevel::Normal:
    rank = std::clamp(rank, kRankNormalMin, kRankNormalMax);
    break;
  case GameLevel::Hard:
  case GameLevel::Extra:
    rank = std::clamp(rank, kRankHardMin, kRankHardMax);
    break;
  case GameLevel::Lunatic:
    rank = std::clamp(rank, kRankLunaticMin, kRankLunaticMax);
    break;
  }
}

void GameSession::ResetRank() {
  switch (level) {
  case GameLevel::Easy:
    rank = kRankEasyDefault;
    break;
  case GameLevel::Normal:
    rank = kRankNormalDefault;
    break;
  case GameLevel::Hard:
  case GameLevel::Extra:
    rank = kRankHardDefault;
    break;
  case GameLevel::Lunatic:
    rank = kRankLunaticDefault;
    break;
  }
}
