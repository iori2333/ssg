///
/// GameManager — rank and session state
///

#include <algorithm>

#include "game_manager.h"

#include "data/gfx_manager.h"

#include "core/gian.h"

GameLevel GameManager::EffectiveLevel() const {
  switch (level) {
  case GameLevel::EASY:
    return (rank < 20 * 256) ? GameLevel::EASY : GameLevel::NORMAL;
  case GameLevel::NORMAL:
    if (rank < 20 * 256) return GameLevel::EASY;
    if (rank < 36 * 256) return GameLevel::NORMAL;
    return GameLevel::HARD;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    if (rank < 36 * 256) return GameLevel::NORMAL;
    if (rank < 44 * 256) return GameLevel::HARD;
    return GameLevel::LUNATIC;
  case GameLevel::LUNATIC:
    if (rank < 44 * 256) return GameLevel::HARD;
    return GameLevel::LUNATIC;
  }
  return level;
}

void GameManager::AddRank(int n) {
  if (stage == GameStage::EXTRA) {
    if (n > 0) {
      rank += (std::max)(+1, (n / 4));
    } else if (n < 0) {
      rank += (std::min)(-1, (n / 10));
    }
  } else {
    rank += n;
  }

  switch (level) {
  case GameLevel::EASY:
    if (rank < 0) rank = 0;
    if (rank > 24 * 256) rank = 24 * 256;
    break;
  case GameLevel::NORMAL:
    if (rank < 16 * 256) rank = 16 * 256;
    if (rank > 40 * 256) rank = 40 * 256;
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    if (rank < 32 * 256) rank = 32 * 256;
    if (rank > 48 * 256) rank = 48 * 256;
    break;
  case GameLevel::LUNATIC:
    if (rank < 40 * 256) rank = 40 * 256;
    if (rank > 64 * 256) rank = 64 * 256;
    break;
  }
}

void GameManager::ResetRank() {
  switch (level) {
  case GameLevel::EASY:
    rank = 16 * 256;
    break;
  case GameLevel::NORMAL:
    rank = 32 * 256;
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    rank = 44 * 256;
    break;
  case GameLevel::LUNATIC:
    rank = 60 * 256;
    break;
  }
}
