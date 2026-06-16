/*************************************************************************************************/
/*   FRAGMENT.H   破片処理関数群 */
/*                                                                                               */
/*************************************************************************************************/

#pragma once

#include <array>
#include <cstdint>

//// 破片定数 ////
inline constexpr auto FRAGMENT_MAX = 1000;  // 破片の最大数
inline constexpr auto FRG_EVADE = 0x00;     // かすり(開発中)
inline constexpr auto FRG_SMOKE = 0x01;     // 煙その１
inline constexpr auto FRG_FATCIRCLE = 0x02; // 赤丸...
inline constexpr auto FRG_STAR1 = 0x03;     // お星様１
inline constexpr auto FRG_STAR2 = 0x04;     // お星様２
inline constexpr auto FRG_HIT = 0x05;       // ショットがヒットした
inline constexpr auto FRG_STAR3 = 0x06;
inline constexpr auto FRG_HEART = 0x07; // ハート型

inline constexpr auto FRG_ESCAPE = 0x10;   // 指定座標から逃げる
inline constexpr auto FRG_APPROACH = 0x20; // 指定座標に近づく

//// 破片構造体 ////
struct FragmentData {
  int x, y;      // 現在の座標
  int vx, vy;    // 速度成分 (x64)
  uint8_t count; // フレームカウンタ(０の時は使用していないとする)
  uint8_t cmd;   // どんな破片？
};
// (FRAGMENT_DATA alias removed — use FragmentData directly)

//// 破片用変数 ////
// Fragment[], FragmentPtr → Effects.fragments, Effects.fragment_ptr で直接アクセス

//// 破片関数 ////
// 後方互換 inline wrapper は effect_manager.h の末尾に移動

