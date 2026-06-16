/*************************************************************************************************/
/*   TAMA.H   たまに関する定義とかいろいろ */
/*                                                                                               */
/*************************************************************************************************/

#pragma once

#include "core/entity.h"

///// [更新履歴] /////

/* -> ここから、ちと古いよ(1999...)
 * 変更しなけりゃいけないこと、とか
 * >ox,oy を廃止すること(TAMA.C も変更しなきゃならんし、面倒だな...)
 *
 * > 5/17 (2:43)  : ox,oy を廃止した。
 * 弾の種類によっては自動的にクリッピングをするようにした : 上の奴との関連により
 * .flag の機能を拡張しました(下の定数参照)
 *
 * > 6/13 (8:05)  : どっと単位のクリッピング
 */

////弾定数////
inline constexpr auto TAMA_MAX = (801 * 3); // 弾の最大発生数
inline constexpr auto TAMA_EVADE = 1;       // 弾のかすり値

inline constexpr auto TAMA1_POINT = 10000; // 弾の得点
inline constexpr auto TAMA2_POINT = 15000; // 弾の得点

inline constexpr auto TAMA_HITX = (2 * 64);              // 弾の当たり判定
inline constexpr auto TAMA_HITY = (4 * 64);              // 弾の当たり判定
inline constexpr auto TAMA_EVX_SMALL = ((8 + 8) * 64);   // たま（小）のかすり判定(x)
inline constexpr auto TAMA_EVY_SMALL = ((16 + 8) * 64);  // たま（小）のかすり判定(y)
inline constexpr auto TAMA_EVX_LARGE = ((8 + 16) * 64);  // たま（大）のかすり判定(x)
inline constexpr auto TAMA_EVY_LARGE = ((16 + 16) * 64); // たま（大）のかすり判定(y)

inline constexpr auto TAMA_SMALL = 0x00;  // 弾が小型弾である場合の上位４ビット
inline constexpr auto TAMA_LARGE = 0x10;  // 弾が大型弾である場合の上位４ビット
inline constexpr auto TAMA_ANGLE = 0x20;  // 弾が方向指定系である場合の上位４ビット
inline constexpr auto TAMA_EXTRA = 0x30;  // 弾がエキストラ用である場合の上位４ビット
inline constexpr auto TAMA_EXTRA2 = 0x40; // 弾が「おふだ」である場合の上位４ビット
inline constexpr auto TAMA_REN = 0x04;    // 弾の連射属性
inline constexpr auto TAMA_ZSET = 0x08;   // 弾のサボテン(自機)セット属性
inline constexpr auto TAMASP_RND0 = 0x00; // 速度ランダム無し
inline constexpr auto TAMASP_RND1 = 0x40; // 速度ランダム？？
inline constexpr auto TAMASP_RND2 = 0x80; // 速度ランダム？？
inline constexpr auto TAMASP_RND3 = 0xc0; // 速度ランダム？？

////弾の種類定数(上位４ビットは現在、使用目的がない)////
inline constexpr auto T_NORM = 0x00;     // 通常弾			:(vx,vy)で移動します
inline constexpr auto T_NORM_A = 0x01;   // 加速弾			:rep 加速回数?
inline constexpr auto T_HOMING = 0x02;   // ｎ回ホーミング	:rep ホーミング回数 / a 加速度
inline constexpr auto T_HOMING_M = 0x03; // ｎ％ホーミング	:a 加速度 / vd ホーミング率
inline constexpr auto T_ROLL = 0x04;     // 回転弾			:rep 回転時間 / vd 角速度
inline constexpr auto T_ROLL_A = 0x05;   // 回転弾(加速)		:上の奴 + a 加速度
inline constexpr auto T_ROLL_R = 0x06;   // 回転弾(反転)		:上の奴と同じ
inline constexpr auto T_GRAVITY = 0x07; // 落下弾 :(vx,vy)＆vyに(加速度a)がかかる
inline constexpr auto T_CHANGE = 0x08;   // 角度変更弾		:rep フレームでvdに角度変更
inline constexpr auto T_SBHOMING = 0x09; // サボテン用ホーミング
inline constexpr auto T_SBHBOMB = 0x0a;  // サボテン用ホーミングボム

////弾オプション定数(下位４ビットはオプションの成分指定用)////
inline constexpr auto TOP_NONE = 0x00;  // オプションなし
inline constexpr auto TOP_WAVE = 0x10;  // 波		: 振幅
inline constexpr auto TOP_ROLL = 0x20;  // 回転		: 回転半径
inline constexpr auto TOP_PURU = 0x30;  // ぷるぷる	: ぷるぷる度
inline constexpr auto TOP_REFX = 0x40;  // 反射Ｘ	: 反射回数
inline constexpr auto TOP_REFY = 0x50;  // 反射Ｙ	: 反射回数
inline constexpr auto TOP_REFXY = 0x60; // 反射ＸＹ	: 反射回数
inline constexpr auto TOP_DIV = 0x70;   // 分裂		: 分裂時の弾コマンド
inline constexpr auto TOP_BOMB = 0x80;  // ボム???	: 爆発半径

////弾コマンド定数////
inline constexpr auto TC_WAY = 0x00;   // 扇状発射
inline constexpr auto TC_ALL = 0x01;   // 全方向発射
inline constexpr auto TC_RND = 0x02;   // 基本角セット有りランダム
inline constexpr auto TC_WAYS = 0x04;  // 扇状発射＆連射
inline constexpr auto TC_ALLS = 0x05;  // 全方向発射＆連射
inline constexpr auto TC_RNDS = 0x06;  // 基本角セット有りランダム＆連射
inline constexpr auto TC_WAYZ = 0x08;  // 扇状発射＆サボテンセット
inline constexpr auto TC_ALLZ = 0x09;  // 全方向発射＆サボテンセット
inline constexpr auto TC_RNDZ = 0x0a;  // 基本角サボテンセットランダム
inline constexpr auto TC_WAYSZ = 0x0c; // 扇状発射＆連射＆サボテンセット
inline constexpr auto TC_ALLSZ = 0x0d; // 全方向発射＆連射＆サボテンセット
inline constexpr auto TC_RNDSZ = 0x0e; // 基本角サボテンセットランダム＆連射

////弾エフェクト定数(下位４ビットの使用方法は現在考案中!!)////
inline constexpr auto TE_NONE = 0x00;    // エフェクトなし
inline constexpr auto TE_ROLL1 = 0x10;   // 回転ためエフェクト
inline constexpr auto TE_ROLL2 = 0x20;   // 回転ためエフェクト
inline constexpr auto TE_WARN = 0x30;    // Warning表示
inline constexpr auto TE_ROCK = 0x40;    // ロックオン
inline constexpr auto TE_CIRCLE1 = 0x50; // わっかエフェクト(小->大)
inline constexpr auto TE_CIRCLE2 = 0x60; // わっかエフェクト(大->小)
inline constexpr auto TE_DELETE = 0xf0;  // 消去エフェクト

////弾フラグ定数////
inline constexpr auto TF_NONE = 0x00;   // フラグが立っていない状態
inline constexpr auto TF_CLIP = 0x01;   // 画面外に出ても消去しない
inline constexpr auto TF_EVADE = 0x02;  // 一回かすっている場合
inline constexpr auto TF_DELETE = 0x80; // その弾を消去する

////弾コマンド構造体(安全性アップ)////
struct BulletCommand {
  int x, y; // 弾の発射位置

  uint8_t d;  // 発射角
  uint8_t dw; // 発射幅
  uint8_t n;  // 弾数(ｎ方向に発射)
  uint8_t ns; // 連射数(cmdのsビットがONのときだけ有効)
  uint8_t v;  // 速度(下位６ビット)＆ランダム要素(上位２ビット)
  uint8_t c;  // 弾の色＆形状
  char a;     // 加速度(速度とは単位が違うので注意)

  char vd; // 角速度｜ホーミング率(BYTE にキャスト)

  uint8_t rep;    // 繰り返し回数(回転、ｎ回ホーミング等)
  uint8_t cmd;    // 弾コマンド＆エフェクト
  uint8_t type;   // 弾の種類
  uint8_t option; // 弾の属性(バイブレーション,反射,炸裂,ボム)
};

////弾データ構造体////
struct Bullet {
  int x, y;   // 現在の<表示>座標
  int tx, ty; // 振動系エフェクト使用時の演算用座標
  int vx, vy; // 速度の(X,Y)成分

  int v;  // 速度
  int v0; // 初速度(回転系エフェクト等で使用)
  char a; // 加速度

  uint8_t d;    // 進行角
  uint16_t d16; // 進行角(固定小数点 x256) -> ｎ％ホーミングでのみ使用する
  int8_t vd;    // 角速度

  uint8_t c; // 弾の色＆形状

  uint8_t rep;    // typeによる制御を行う回数
  uint8_t type;   // 弾の種類(通常,加速,ホーミング2,回転3,落下,変更)
  uint8_t option; // 弾の属性(バイブレーション,反射,炸裂,ボム)
  uint8_t effect; // 実行中のエフェクト(なし,ロック,サークル,消去)

  uint16_t count; // フレームカウンタ
  uint8_t flag;   // 弾消去要請フラグ
};

// 後方互換用エイリアス
using TAMA_CMD = BulletCommand;
using TAMA_DATA = Bullet;

////弾の各種変数たち////
// Bullets.bullets, Bullets.command, Bullets.indices_small/large, Bullets.count_small/large で直接アクセス

////弾関数////
// 実装は BulletManager メソッドに移行
// TamaSetForm, TamaSTDForm, TamaSetDeg, TamaSetNum, TamaSetSpd, TamaSetXY → bullet_manager.h に移動

//// かすり用マクロ ////
void evade_addEx(int x, int y, uint8_t n); // かすりゲージを上昇させる

inline void TamaEvadeAdd(TAMA_DATA *t) {
  if (t->flag & TF_EVADE)
    evade_addEx(t->x, t->y, 0);
  else {
    t->flag |= TF_EVADE;
    evade_addEx(t->x, t->y, TAMA_EVADE);
  }
}

template <size_t N>
void Indsort(std::array<uint16_t, N> &indices, uint16_t &count,
             const std::array<TAMA_DATA, N> &entities) {
  Indsort(indices, count, entities,
          [](const TAMA_DATA &t) { return (t.flag & TF_DELETE); });
}

