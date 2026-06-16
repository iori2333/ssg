/*************************************************************************************************/
/*   LASER.H   レーザーに関する処理(反射,ショート) */
/*                                                                                               */
/*************************************************************************************************/

#pragma once

#include "game/coords.h"
#include "platform/graphics_backend.h"
#include <cstdint>

///// [更新履歴] /////

// 2000/02/17 : 新しいシステムに移行開始。無限遠レーザーと完全に分離

/*-> ここからはちょっと古いよ(1999..)
 * (4/3)  10:36 開発開始
 * (4/6)  12:00 ついにポリゴン＆クリッピング関数が完成。描画はいつ出来るのか？
 * (4/7)  12:02 全てのレーザーを同じ構造体で扱う事にした
 * (4/8)   7:23 無限遠レーザーの制作
 * (4/9)   2:01 反射レーザーを一応打ち込み終わる
 * (4/9)   2:59 反射レーザー完成
 * (4/11) 14:05 ショート＆反射レーザーの当たり判定完成
 * (4/11) 15:17 リフレクターのヒットチェックを強化(バグは消えたが遅くなった)
 *
 * (9/23) 16:18 ライン描画、ＥＣＬ対応などが完了
 */

////レーザー定数////
inline constexpr auto LASER_MAX = 1000; // レーザーの最大発生本数

////レーザー発動コマンド構造体////
struct LaserCommand {
  int x, y; // 始点の座標
  int v;    // レーザーの初速度

  int w;  // レーザーの太さ        (x64座標を使用する)
  int l;  // レーザーの長さ最終値  (x64座標を使用する)
  int l2; // レーザーの発射位置補正(x64...)

  uint8_t d;  // 発射角
  uint8_t dw; // 発射幅

  uint8_t n; // レーザーの本数
  uint8_t c; // レーザーの色

  char a;       // 加速度(つかうのかな???)
  uint8_t cmd;  // レーザー発動コマンド(ほとんど弾と同じかも)
  uint8_t type; // ショート、無限遠など
  uint8_t notr; // 反射しないリフレクターの番号
};
using LASER_CMD = LaserCommand;

////レーザー用構造体////
inline constexpr auto LF_DELETE = 0x80; // レーザーを消去する(処理対象から外す)

struct LASER_DATA {
  int x, y;   // 現在の始点
  int vx, vy; // 速度の(X,Y)成分
  int lx, ly; // 表示座標の加算値(長さ)
  int wx, wy; // 表示座標の加算値(太さ)
  int v;      // 速度

  VERTEX_XY p[4]; // 表示する座標

  char a;    // 加速度(つかうのか??)
  uint8_t d; // 進行方向

  int w, wmax; // 太さ
  int l, lmax; // 現在の長さ、長さの最終値
  int ltemp;   // 反射レーザー専用変数(発射＆ヒットの場合にのみ使用)

  uint16_t count; // フレームカウンタ
  uint8_t c;      // 色
  uint8_t type;   // 種類
  uint8_t flag;   // 消去要請フラグ等(エフェクト含む)
  uint8_t notr;   // 反射しないリフレクターの番号
  uint8_t evade;  // かすり用フラグ
};

/*
////反射物(鏡?) 構造体////
typedef struct{
        int		x,y;		// 反射物の中心座標

        uint32_t	l;	//
リフレクターの長さ(中心から先端まで,つまり全体でl*2) uint8_t	d;	//
反射物の角度(0 <= d < 128) } REFLECTOR;
*/

////レーザー関数////
// 後方互換 inline wrapper は laser_manager.h 末尾に移動
// 実装は LaserManager メソッドに移行

////レーザーの各種変数たち////
extern LaserCommand& LaserCmd; // 標準レーザーコマンド構造体
extern uint16_t& LaserNow;     // レーザーの本数
extern std::array<LASER_DATA, LASER_MAX>& Laser;    // レーザー格納用構造体
extern std::array<uint16_t, LASER_MAX>& LaserInd;   // レーザー順番維持用配列
// extern REFLECTOR	Reflector[RT_MAX];		// 反射物構造体
// extern uint16_t	ReflectorNow;	// 反射物の個数

