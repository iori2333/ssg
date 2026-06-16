/*                                                                           */
/*   LLaser.h   長いレーザーの処理                                           */
/*                                                                           */
/*                                                                           */

#pragma once

// 更新履歴 //
// 2000/05/29 : ８ビットモード描画関連のＢｕｇＦｉｘ
// 2000/03/22 : レーザー関数のＩＤの意味がレーザー配列のＩＤからその敵が
//            : 発射しているレーザーの何番目か、に変更された

#include "enemy/ENEMY.h"
#include "platform/graphics_backend.h"

//// レーザー用定数２ ////
inline constexpr auto LLASER_MAX = 20;
inline constexpr auto LLASER_EVADE = 1; // レーザーのかすり値

//// レーザーの種類定数２
inline constexpr auto LLS_LONG = 0x00;
inline constexpr auto LLS_LONGY = 0x01;
inline constexpr auto LLS_SETDEG = 0x02;
inline constexpr auto LLS_LONGZ = 0x03; // 自機セット

//// レーザーフラグ２ ////
inline constexpr auto LLF_DISABLE = 0x00; // レーザーが使用されていない
inline constexpr auto LLF_NORM = 0x01; // レーザーが完全に開ききった
inline constexpr auto LLF_OPEN = 0x02; // レーザを開いている
inline constexpr auto LLF_CLOSE = 0x04; // レーザーを閉じている
inline constexpr auto LLF_CLOSEL = 0x08; // レーザーをライン状態にする
inline constexpr auto LLF_LINE = 0x10; // レーザーはライン状態

//// レーザー発動コマンド構造体２ ////
struct LongLaserCommand {
  ENEMY_DATA *e; // 敵データへのポインタ

  int dx, dy; // レーザーの発射座標ずらし値
  int v;      // レーザーの速度

  int w; // レーザーの太さ最終値

  uint8_t d; // レーザー発射角

  uint8_t c;    // レーザーの色
  uint8_t type; // レーザーの種類
};
using LLASER_CMD = LongLaserCommand;

//// レーザー用構造体２ ////
struct LongLaserData {
  ENEMY_DATA
  *e; // 敵データへのポインタ(ここら辺でボスでも雑魚でも発射できるように)

  int x, y;       // 現在の表示座標
  int dx, dy;     // 敵データからのずらし値(x64)
  int lx, ly;     // レーザー円の中心座標までのベクトル(Grp)
  int infx, infy; // 仮の無限遠へのベクトル(Grp)
  int wx, wy;     // レーザー幅(Grp)

  int w, wmax; // 幅、最大幅(x64)
  int v;

  uint32_t count; // フレームカウンタ

  VERTEX_XY p[4]; // 座標保持用のポインタ(Grp)

  uint8_t d; // レーザーの発射角
  uint8_t c; // レーザーの色

  uint8_t flag;    // レーザーの状態
  uint8_t type;    // レーザーの種類
  uint8_t EnemyID; // 敵から見た番号
};
using LLASER_DATA = LongLaserData;

//// レーザー関数２ ////
// 後方互換 inline wrapper は laser_manager.h 末尾に移動
// 実装は LaserManager メソッドに移行

//// レーザー変数２ ////
extern std::array<LongLaserData, LLASER_MAX>& LLaser;
extern LongLaserCommand& LLaserCmd;

