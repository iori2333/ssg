///
/// GameManager — rank and session state
///

#include <algorithm>
#include <string_view>

#include "game_manager.h"
#include "level.h"

constexpr auto RANK_EASY_DEFAULT = 20 * 256;
constexpr auto RANK_EASY_MIN = 16 * 256;
constexpr auto RANK_EASY_MAX = 32 * 256;

constexpr auto RANK_NORMAL_DEFAULT = 24 * 256;
constexpr auto RANK_NORMAL_MIN = 22 * 256;
constexpr auto RANK_NORMAL_MAX = 40 * 256;

constexpr auto RANK_HARD_DEFAULT = 36 * 256;
constexpr auto RANK_HARD_MIN = 34 * 256;
constexpr auto RANK_HARD_MAX = 52 * 256;

constexpr auto RANK_LUNATIC_DEFAULT = 48 * 256;
constexpr auto RANK_LUNATIC_MIN = 44 * 256;
constexpr auto RANK_LUNATIC_MAX = 64 * 256;

std::string_view GameManager::LevelName() const {
  switch (level) {
  case GameLevel::EASY:
    return "Easy";
  case GameLevel::NORMAL:
    return "Normal";
  case GameLevel::HARD:
    return "Hard";
  case GameLevel::EXTRA:
    return "Extra";
  case GameLevel::LUNATIC:
    return "Lunatic";
  }
}

GameLevel GameManager::EffectiveLevel() const {
  switch (level) {
  case GameLevel::EASY:
    if (rank <= RANK_EASY_MAX) {
      return GameLevel::EASY;
    }
    return GameLevel::NORMAL;
  case GameLevel::NORMAL:
    if (rank < RANK_NORMAL_MIN) {
      return GameLevel::EASY;
    }
    if (rank <= RANK_NORMAL_MAX) {
      return GameLevel::NORMAL;
    }
    return GameLevel::HARD;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    if (rank < RANK_HARD_MIN) {
      return GameLevel::NORMAL;
    }
    if (rank <= RANK_HARD_MAX) {
      return GameLevel::HARD;
    }
    return GameLevel::LUNATIC;
  case GameLevel::LUNATIC:
    if (rank < RANK_LUNATIC_MIN) {
      return GameLevel::HARD;
    }
    return GameLevel::LUNATIC;
  }
}

void GameManager::Update(uint32_t stage_frame) {
  if (stage_frame % 60 == 0) {
    AddRank(1);
  }
}

void GameManager::AddRank(int n) {
  rank += n;

  switch (level) {
  case GameLevel::EASY:
    rank = std::clamp(rank, RANK_EASY_MIN, RANK_EASY_MAX);
    break;
  case GameLevel::NORMAL:
    rank = std::clamp(rank, RANK_NORMAL_MIN, RANK_NORMAL_MAX);
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    rank = std::clamp(rank, RANK_HARD_MIN, RANK_HARD_MAX);
    break;
  case GameLevel::LUNATIC:
    rank = std::clamp(rank, RANK_LUNATIC_MIN, RANK_LUNATIC_MAX);
    break;
  }
}

void GameManager::ResetRank() {
  switch (level) {
  case GameLevel::EASY:
    rank = RANK_EASY_DEFAULT;
    break;
  case GameLevel::NORMAL:
    rank = RANK_NORMAL_DEFAULT;
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    rank = RANK_HARD_DEFAULT;
    break;
  case GameLevel::LUNATIC:
    rank = RANK_LUNATIC_DEFAULT;
    break;
  }
}
