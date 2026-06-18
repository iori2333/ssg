/*                                                                           */
/*   Score.h   スコア入出力関数                                              */
/*                                                                           */
/*                                                                           */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

///// [ 定数 ] /////
inline constexpr std::size_t NR_NAME_LEN =
    9; // ネームレジストの名前の長さ('\0' 含む)
inline constexpr std::size_t NR_RANK_MAX = 5; // 順位付け(Save)される最大数

///// [構造体] /////

struct NrNameData {
  char Name[NR_NAME_LEN]; // 名前
  int64_t Score;          // スコア
  uint32_t Evade;         // かすり回数
  uint8_t Stage;          // ステージ
  uint8_t Weapon;         // 装備品
};
// (NrNameData alias removed — use NrNameData directly)

struct NrScoreData {
  NrNameData Easy[NR_RANK_MAX];    // 難易度：Ｅａｓｙ
  NrNameData Normal[NR_RANK_MAX];  // 難易度：Ｎｏｒｍａｌ
  NrNameData Hard[NR_RANK_MAX];    // 難易度：Ｈａｒｄ
  NrNameData Lunatic[NR_RANK_MAX]; // 難易度：Ｌｕｎａｔｉｃ
  NrNameData Extra[NR_RANK_MAX];   // 難易度：Ｅｘｔｒａ
};
// (NR_SCORE_DATA alias removed — use NrScoreData directly)

struct NrScoreString {
  uint8_t Rank;     // 実際の順位(ある順位が複数ある場合の対策)
  int x, y;         // 描画用座標
  bool bMoveEnable; // 移動可能か？

  char Name[NR_NAME_LEN];          // 名前
  std::string Score;                // 得点
  std::string Evade;                // かすり
  std::string Stage;                // ステージ
  uint8_t Weapon;                   // 装備
};
// (NR_SCORE_STRING alias removed — use NrScoreString directly)

///// [ 関数 ] /////
// 後方互換 inline wrapper は score_manager.h 末尾に移動
// 実装は ScoreManager メソッドに移行
