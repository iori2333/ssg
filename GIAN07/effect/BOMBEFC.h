/*
 *   BombEfc.h   : 爆発系エフェクト
 *
 */

#pragma once

#include <cstdint>

/***** [ 定数 ] *****/
inline constexpr auto EXBOMB_MAX = 3;      // エフェクトの同時発生数
inline constexpr auto EXBOMB_STD = 0;      // よくあるタイプの爆発(??)
inline constexpr auto EXBOMB_OBJMAX = 200; // エフェクト補助用オブジェクトの個数

/***** [構造体] *****/
struct SpObj {
  int x, y;
  int vx, vy;
  uint8_t d;
};

struct BombEffectCtrl {
  int x, y;       // エフェクトの中心座標
  bool bIsUsed;   // この構造体は使用中か
  uint32_t count; // フレームカウンタ

  SpObj Obj[EXBOMB_OBJMAX]; // エフェクト補助用オブジェクト

  uint8_t type; // エフェクトの種類
};
using BombEfcCtrl = BombEffectCtrl;

/***** [関数プロトタイプ] *****/
// 後方互換 inline wrapper は effect_manager.h の末尾に移動


