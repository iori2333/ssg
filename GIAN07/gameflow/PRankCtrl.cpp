/*                                                                           */
/*   PRankCtrl.cpp   プレイランク管理                                        */
/*                                                                           */
/*                                                                           */

#include "PRankCtrl.h"
#include "GIAN.h"
#include "LEVEL.h"

// PlayRank → rank_manager.cpp に移動

// 難易度の許容範囲内でプレイランクを増減する
void RankManager::Add(int n) {
  // イージー 　　　0 ～ 24
  // ノーマル　　　16 ～ 40
  // ハード　　 　　32 ～ 48
  // ルナティック  40 ～ 64

  // 難易度を変化させる //
  if (GameStage == GRAPH_ID_EXSTAGE) {
    if (n > 0) {
      this->state.Rank += (std::max)(+1, (n / 4));
    } else if (n < 0) {
      this->state.Rank += (std::min)(-1, (n / 10));
    }
  } else {
    this->state.Rank += n;
  }

  // この分岐に関しては、基本的にコンフィグの値に基づく //
  switch (GameLevel) {
  case (GAME_EASY):
    if (this->state.Rank < 0)
      this->state.Rank = 0;
    else if (this->state.Rank > 24 * 256)
      this->state.Rank = 24 * 256;

    if (this->state.Rank < 20 * 256)
      this->state.GameLevel = GAME_EASY;
    else
      this->state.GameLevel = GAME_NORMAL;
    break;

  case (GAME_NORMAL):
    if (this->state.Rank < 16 * 256)
      this->state.Rank = 16 * 256;
    else if (this->state.Rank > 40 * 256)
      this->state.Rank = 40 * 256;

    if (this->state.Rank < 20 * 256)
      this->state.GameLevel = GAME_EASY;
    else if (this->state.Rank < 36 * 256)
      this->state.GameLevel = GAME_NORMAL;
    else
      this->state.GameLevel = GAME_HARD;
    break;

  case (GAME_HARD):
    if (this->state.Rank < 32 * 256)
      this->state.Rank = 32 * 256;
    else if (this->state.Rank > 48 * 256)
      this->state.Rank = 48 * 256;

    if (this->state.Rank < 36 * 256)
      this->state.GameLevel = GAME_NORMAL;
    else if (this->state.Rank < 44 * 256)
      this->state.GameLevel = GAME_HARD;
    else
      this->state.GameLevel = GAME_LUNATIC;
    break;

  case (GAME_LUNATIC):
    if (this->state.Rank < 40 * 256)
      this->state.Rank = 40 * 256;
    else if (this->state.Rank > 64 * 256)
      this->state.Rank = 64 * 256;

    if (this->state.Rank < 44 * 256)
      this->state.GameLevel = GAME_HARD;
    else
      this->state.GameLevel = GAME_LUNATIC;
    break;

    // case(GAME_EXTRA):
    // break;
  }
}

// 現在の難易度に応じてプレイランクを初期化
void RankManager::Reset(void) {
  this->state.GameLevel = GameLevel;

  switch (GameLevel) {
  case (GAME_EASY):
    this->state.Rank = 16 * 256;
    break;
  case (GAME_NORMAL):
    this->state.Rank = 32 * 256;
    break;
  case (GAME_HARD):
    this->state.Rank = 44 * 256;
    break;
  case (GAME_LUNATIC):
    this->state.Rank = 60 * 256;
    break;
    // case(GAME_EXTRA):		break;
  }
}
