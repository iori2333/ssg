/*                                                                           */
/*   PRankCtrl.h   プレイランク管理                                          */
/*                                                                           */
/*                                                                           */

#pragma once

#include <cstdint>

///// [構造体] /////
struct PlayRankState {
  uint8_t GameLevel; // 方向数も関係する難易度変化
  int Rank;          // 弾の速度変化に関する値
};
using PlayRankInfo = PlayRankState;

///// [グローバル変数] /////
extern PlayRankState& PlayRank;

///// [ 関数 ] /////
void PlayRankAdd(int n);  // 難易度の許容範囲内でプレイランクを増減する
void PlayRankReset(void); // 現在の難易度に応じてプレイランクを初期化
