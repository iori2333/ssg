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
// 後方互換 inline wrapper は rank_manager.h 末尾に移動
// 実装は RankManager メソッドに移行
