///
/// GameSession - fixed rank and session state
///

#include <utility>

#include "game_rules.h"
#include "game_session.h"

namespace {

constexpr auto kRankEasy = 20 * 256;

constexpr auto kRankNormal = 24 * 256;

constexpr auto kRankHard = 36 * 256;

constexpr auto kRankLunatic = 48 * 256;

} // namespace

int GameSession::Rank() const {
  switch (level) {
  case GameLevel::Easy:
    return kRankEasy;
  case GameLevel::Normal:
    return kRankNormal;
  case GameLevel::Hard:
  case GameLevel::Extra:
    return kRankHard;
  case GameLevel::Lunatic:
    return kRankLunatic;
  }
  return kRankNormal;
}

void GameSession::AdvanceStage() {
  if (stage < StageId::Stage6) {
    stage = static_cast<StageId>(std::to_underlying(stage) + 1);
  }
}
