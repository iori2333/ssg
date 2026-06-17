/*                                                                           */
/*   HomingL.h   長いレーザーの処理                                          */
/*                                                                           */
/*                                                                           */

#pragma once

#include "core/point.h"

///// [ 定数 ] /////
inline constexpr auto HLASER_MAX = 162;
inline constexpr auto HLASER_LEN = 7; // 描画枚数..
inline constexpr auto HLASER_SECTION = 4; // 読み込み幅

inline constexpr auto HL_NONE = 0; // ただ進むだけ
inline constexpr auto HL_TYPE1 = 1; // その１

inline constexpr auto HLS_NORM = 0x00; // ホーミングレーザー通常
inline constexpr auto HLS_CLEAR = 0x01; // ホーミングレーザー消去中
inline constexpr auto HLS_DEAD = 0xff; // ホーミングレーザー削除要請

///// [構造体] /////

// ホーミングレーザー //
struct HomingLaserData {
  int Current; // 現在の先頭
  int v;       // 速度
  int a;       // 加速度

  uint32_t Count; // フレームカウンタ

  uint8_t Type;  // 種類(加速＆ホーミングタイプ)
  uint8_t State; // 状態
  uint8_t c;     // 色
  uint8_t Left;  // 残りホーミング回数

  HomingLaserData *Next;                   // 次のレーザーへのポインタ
  DegPoint p[HLASER_LEN * HLASER_SECTION]; // 頂点キュー(ExDef.h)
};
// (HLaserData alias removed — use HomingLaserData directly)

// ホーミングレーザーセット情報 //
struct HomingLaserInfo {
  int x, y; // 中心座標

  uint8_t d;  // 角度
  uint8_t dw; // 角度の開き
  uint8_t n;  // 本数

  uint8_t c;    // 色
  uint8_t type; // 種類
};
// (HLaserInfo alias removed — use HomingLaserInfo directly)

///// [グローバル変数] /////
// Lasers.homing_count, Lasers.homing_cmd で直接アクセス

///// [関数プロトタイプ] /////
// 後方互換 inline wrapper は laser_manager.h 末尾に移動
// 実装は LaserManager メソッドに移行

