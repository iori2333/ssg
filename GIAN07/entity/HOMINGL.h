/*                                                                           */
/*   HomingL.h   長いレーザーの処理                                          */
/*                                                                           */
/*                                                                           */

#pragma once

#include "game/EXDEF.h"

///// [ 定数 ] /////
inline constexpr auto HLASER_MAX = 162;
inline constexpr auto HLASER_LEN = 7;     // 描画枚数..
inline constexpr auto HLASER_SECTION = 4; // 読み込み幅

inline constexpr auto HL_NONE = 0;  // ただ進むだけ
inline constexpr auto HL_TYPE1 = 1; // その１

inline constexpr auto HLS_NORM = 0x00;  // ホーミングレーザー通常
inline constexpr auto HLS_CLEAR = 0x01; // ホーミングレーザー消去中
inline constexpr auto HLS_DEAD = 0xff;  // ホーミングレーザー削除要請

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
using HLaserData = HomingLaserData;

// ホーミングレーザーセット情報 //
struct HomingLaserInfo {
  int x, y; // 中心座標

  uint8_t d;  // 角度
  uint8_t dw; // 角度の開き
  uint8_t n;  // 本数

  uint8_t c;    // 色
  uint8_t type; // 種類
};
using HLaserInfo = HomingLaserInfo;

///// [グローバル変数] /////
extern uint16_t &HLaserNow;        // ホーミングレーザーの本数
extern HomingLaserInfo &HLaserCmd; // ホーミングレーザーセット用データ

///// [関数プロトタイプ] /////
void HLaserInit(void);                   // ホーミングレーザーの初期化を行う
void HLaserSet(const HLaserInfo *hinfo); // ホーミングレーザーをセットする
void HLaserMove(void);                   // ホーミングレーザーを動作させる
void HLaserDraw(void);                   // ホーミングレーザーを描画する
void HLaserClear(void); // ホーミングレーザーに消去エフェクトをセット
