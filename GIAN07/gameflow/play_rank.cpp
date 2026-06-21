///
/// PlayRank - Play rank management
///

#include "play_rank.h"

#include "core/gian.h"
#include "core/level.h"

// PlayRank moved to rank_manager.cpp

// Increase/decrease play rank within difficulty range
void RankManager::Add(int n) {
  // Easy           0 .. 24
  // Normal         16 .. 40
  // Hard           32 .. 48
  // Lunatic        40 .. 64

  // Change difficulty
  if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
    if (n > 0) {
      state.Rank += (std::max)(+1, (n / 4));
    } else if (n < 0) {
      state.Rank += (std::min)(-1, (n / 10));
    }
  } else {
    state.Rank += n;
  }

  // This branch is based on config values
  switch (GameState.game_level) {
  case GameLevel::EASY:
    if (state.Rank < 0) {
      state.Rank = 0;
    } else if (state.Rank > 24 * 256) {
      state.Rank = 24 * 256;
    }

    if (state.Rank < 20 * 256) {
      state.GameLevel = GameLevel::EASY;
    } else {
      state.GameLevel = GameLevel::NORMAL;
    }
    break;

  case GameLevel::NORMAL:
    if (state.Rank < 16 * 256) {
      state.Rank = 16 * 256;
    } else if (state.Rank > 40 * 256) {
      state.Rank = 40 * 256;
    }

    if (state.Rank < 20 * 256) {
      state.GameLevel = GameLevel::EASY;
    } else if (state.Rank < 36 * 256) {
      state.GameLevel = GameLevel::NORMAL;
    } else {
      state.GameLevel = GameLevel::HARD;
    }
    break;

  case GameLevel::HARD:
  case GameLevel::EXTRA:
    if (state.Rank < 32 * 256) {
      state.Rank = 32 * 256;
    } else if (state.Rank > 48 * 256) {
      state.Rank = 48 * 256;
    }

    if (state.Rank < 36 * 256) {
      state.GameLevel = GameLevel::NORMAL;
    } else if (state.Rank < 44 * 256) {
      state.GameLevel = GameLevel::HARD;
    } else {
      state.GameLevel = GameLevel::LUNATIC;
    }
    break;

  case GameLevel::LUNATIC:
    if (state.Rank < 40 * 256) {
      state.Rank = 40 * 256;
    } else if (state.Rank > 64 * 256) {
      state.Rank = 64 * 256;
    }

    if (state.Rank < 44 * 256) {
      state.GameLevel = GameLevel::HARD;
    } else {
      state.GameLevel = GameLevel::LUNATIC;
    }
    break;

    // case(GAME_EXTRA):
    // break;
  }
}

// Initialize play rank based on current difficulty
void RankManager::Reset() {
  state.GameLevel = GameState.game_level;

  switch (GameState.game_level) {
  case GameLevel::EASY:
    state.Rank = 16 * 256;
    break;
  case GameLevel::NORMAL:
    state.Rank = 32 * 256;
    break;
  case GameLevel::HARD:
  case GameLevel::EXTRA:
    state.Rank = 44 * 256;
    break;
  case GameLevel::LUNATIC:
    state.Rank = 60 * 256;
    break;
    // case(GAME_EXTRA):		break;
  }
}
