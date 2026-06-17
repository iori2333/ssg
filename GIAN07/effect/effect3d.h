/*                                                                           */
/*   Effect3D.h   ３Ｄエフェクトの処理                                       */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////
// 2000/05/31 : 開発開始

///// [ヘッダファイル] /////
#include "game/coords.h"
#include <cstdint>
#include <span>

///// [ 定数 ] /////
inline constexpr auto STG4ROCK_STDMOVE = 0;  // 普通のスクロールね
inline constexpr auto STG4ROCK_ACCMOVE1 = 1; // 加速有りスクロール(1)
inline constexpr auto STG4ROCK_ACCMOVE2 = 2; // 加速有りスクロール(2)
inline constexpr auto STG4ROCK_3DMOVE = 3;   // ３Ｄ回転
inline constexpr auto STG4ROCK_LEAVE = 4;    // 一時的に岩を消去する
inline constexpr auto STG4ROCK_END = 5;      // エフェクト終了

///// [ 構造体 ] /////
struct Point3D {
  WORLD_COORD x, y, z;
};

struct LineList3D {
  PIXEL_POINT center;       /* 頂点の座標の補正用 */
  std::span<WORLD_POINT> p; /* 頂点の座標         */

  uint8_t DegX, DegY, DegZ; /* 各軸に対する回転角 */
};

struct Circle3D {
  int ox, oy;
  int r;
  uint8_t deg;
  uint8_t n;
};

struct Deg3D {
  int dx;
  int dy;
  int dz;
};

struct Cube3D {
  Point3D p;
  Deg3D d;
  int l;
};

struct Star2D {
  int x, y;
  int vy;
};

// 雲管理用構造体 //
struct Cloud2D {
  int x, y;     // ｘ６４座標
  int vy;       // 速度のｙ成分(ｙしかないけど)
  uint8_t type; // 雲の種類
};

// 非・汎用２Ｄ正方形ワイヤーフレーム //
struct WFLine2D {
  int ox, oy; // 中心座標
  int w;      // 正方形の一辺の長さ
  uint8_t d;  // 正方形の傾き角度
};

// 偽ＥＣＬ羅列管理用構造体 //
struct FakeECLString {
  int SrcX, SrcY; // 元画像の基準となるＸ＆Ｙ座標
  int x, y;       // 現在の座標x64
  int vx, vy;     // 現在の速度成分x64
};

// 岩管理用構造体 //
struct Rock3D {
  int x, y, z; // 現在の座標
  int vx, vy;  // 速度XY成分(2D-平面上)

  uint32_t count; // カウンタ
  int v;          // 速度

  char a;        // 加速度
  uint8_t d;     // 角度(2D-平面上)
  uint8_t GrpID; // グラフィックＩＤ
  uint8_t State; // 現在の状態
};

///// [ 関数 ] /////
// Utility functions (not manager methods)
void InitLineList3D(std::span<LineList3D> w);
void DrawLineList3D(std::span<const LineList3D> w);
void MoveWarningR(char count);

// 後方互換 inline wrapper は effect_manager.h の末尾に移動
// 実装は EffectManager メソッドに移行
