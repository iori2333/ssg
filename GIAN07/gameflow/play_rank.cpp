/*                                                                           */
/*   PRankCtrl.cpp   プレイランク管理                                        */
/*                                                                           */
/*                                                                           */

#include "play_rank.h"
#include "gian.h"
#include "level.h"

// PlayRank → rank_manager.cpp に移動

// 難易度の許容範囲内でプレイランクを増減する
void RankManager::Add(int n) {
  // イージー 　　　0 ～ 24
  // ノーマル　　　16 ～ 40
  // ハード　　 　　32 ～ 48
  // ルナティック  40 ～ 64

  // 難易度を変化させる //
  if (GameState.game_stage == GRAPH_ID_EXSTAGE) {
    if (n > 0) {
      state.Rank += (std::max)(+1, (n / 4));
    } else if (n < 0) {
      state.Rank += (std::min)(-1, (n / 10));
    }
  } else {
    state.Rank += n;
  }

  // この分岐に関しては、基本的にコンフィグの値に基づく //
  switch (GameState.game_level) {
  case (GAME_EASY):
    if (state.Rank < 0)
      state.Rank = 0;
    else if (state.Rank > 24 * 256)
      state.Rank = 24 * 256;

    if (state.Rank < 20 * 256)
      state.GameLevel = GAME_EASY;
    else
      state.GameLevel = GAME_NORMAL;
    break;

  case (GAME_NORMAL):
    if (state.Rank < 16 * 256)
      state.Rank = 16 * 256;
    else if (state.Rank > 40 * 256)
      state.Rank = 40 * 256;

    if (state.Rank < 20 * 256)
      state.GameLevel = GAME_EASY;
    else if (state.Rank < 36 * 256)
      state.GameLevel = GAME_NORMAL;
    else
      state.GameLevel = GAME_HARD;
    break;

  case (GAME_HARD):
    if (state.Rank < 32 * 256)
      state.Rank = 32 * 256;
    else if (state.Rank > 48 * 256)
      state.Rank = 48 * 256;

    if (state.Rank < 36 * 256)
      state.GameLevel = GAME_NORMAL;
    else if (state.Rank < 44 * 256)
      state.GameLevel = GAME_HARD;
    else
      state.GameLevel = GAME_LUNATIC;
    break;

  case (GAME_LUNATIC):
    if (state.Rank < 40 * 256)
      state.Rank = 40 * 256;
    else if (state.Rank > 64 * 256)
      state.Rank = 64 * 256;

    if (state.Rank < 44 * 256)
      state.GameLevel = GAME_HARD;
    else
      state.GameLevel = GAME_LUNATIC;
    break;

    // case(GAME_EXTRA):
    // break;
  }
}

// 現在の難易度に応じてプレイランクを初期化
void RankManager::Reset(void) {
  state.GameLevel = GameState.game_level;

  switch (GameState.game_level) {
  case (GAME_EASY):
    state.Rank = 16 * 256;
    break;
  case (GAME_NORMAL):
    state.Rank = 32 * 256;
    break;
  case (GAME_HARD):
    state.Rank = 44 * 256;
    break;
  case (GAME_LUNATIC):
    state.Rank = 60 * 256;
    break;
    // case(GAME_EXTRA):		break;
  }
}
