/*************************************************************************************************/
/*   ENEMY.h   敵の管理とか発生制御等 */
/*                                                                                               */
/*************************************************************************************************/

#pragma once

///// [ 更新履歴 ] /////

// 2000/10/17 : プレイランクの変数を参照するのを間違えていた(ConfigDat
// のほうを参照していた) 2000/03/22 : LLaser
// の処理を追加(実際の発射は構造体そのものに直接代入する事で行う) 2000/02/25 :
// 敵の当たり判定チェック用関数 enemy_damage() を追加 2000/02/22 :
// 敵のクリッピング範囲を変更した。

#include "ECL.h"
#include "bullet/laser.h"
#include "bullet/bullet.h"
#include "platform/buffer.h"

//// 敵定数 ////
inline constexpr uint16_t ENEMY_MAX = 50; // 敵の最大発生数

// 敵状態フラグ
inline constexpr uint8_t EF_DRAW   = 0x01; // 敵を描画するか
inline constexpr uint8_t EF_CLIP   = 0x02; // 敵が画面外に出たとき消去するか
inline constexpr uint8_t EF_DAMAGE = 0x04; // 敵にダメージを与えられるか
inline constexpr uint8_t EF_HITSB  = 0x08; // 敵と自機は接触するか
inline constexpr uint8_t EF_RLCHG  = 0x10; // ＥＣＬ左右反転を有効にするか
inline constexpr uint8_t EF_BOMB   = 0x20; // 敵が爆発中である
inline constexpr uint8_t EF_DELETE = 0x40; // 敵をこのターン中に消去する

inline constexpr int ENEMY_BOMB_SPD = 4;

//// ホーミング定数 ////
inline constexpr int HOMING_DUMMY = (500 * 64); // 敵をホーミングしない場合のダミー値

////アニメーション定数////
inline constexpr uint8_t ANIME_MAX    = 50; // アニメーションの種類
inline constexpr uint8_t ANIMEPTN_MAX = 16; // アニメーションパターンの最大値
inline constexpr uint8_t ANM_NORM     = 0x00; // 普通のアニメーション
inline constexpr uint8_t ANM_DEG      = 0x01; // 角度でアニメーションする
inline constexpr uint8_t ANM_STOP     = 0x02; // 最終パターンで静止する

//// 割り込みベクタ構造体 ////
struct InterruptVector {
  uint32_t vect; // 割り込みベクタ(0 なら無効)
  int value;     // 比較値
};

//// 敵データ構造体 ////
struct EnemyData {
  WORLD_COORD x, y; // 表示座標
  int vx, vy;       // 速度の(x,y)成分 x64系

  int v; // 速度成分 x64系

  uint32_t hp;        // 残り体力(大きすぎるか？)
  uint32_t item;      // アイテムその他に使用か？
  uint32_t cmd;       // ECLコマンドの絶対アドレス(DOS版との大きな変更点)
  uint32_t count;     // 多目的フレームカウンタ
  uint32_t call_addr; // RET 実行後にジャンプするアドレス

  uint32_t score;   // 得点(時間による得点変動対応か？)
  uint32_t evscore; // かすり得点用

  uint32_t IntTimer; // 割り込みようタイマー

  uint32_t GR[ECLREG_MAX];              // 変数用レジスタ
  InterruptVector Vect[ECLVECT_MAX];    // 割り込みベクタ

  uint16_t g_width;  // グラフィックの幅  /2*64(当たり判定にも使用)
  uint16_t g_height; // グラフィックの高さ/2*64(上に同じ)
  uint16_t rep_c;    // REP 命令用カウンタ
  uint16_t cmd_c;    // 実行中コマンドの繰り返し回数
  uint16_t anm_c;    // アニメーションカウンタ

  uint8_t d;         // 進行角 256
  char vd;           // 角速度 128
  uint8_t amp;       // 振幅   256
  uint8_t anm_ptn;   // 使用しているアニメーションパターン
  uint8_t anm_ptnEx; // ダメージ中のアニメーションパターン
  char anm_sp;       // アニメーションスピード
  uint8_t IsDamaged; // ダメージを受けたか

  uint8_t flag;   // 敵状態フラグ(状況によってサイズ変更のこと)
  uint8_t tama_c; // 弾発射用カウンタ
  uint8_t t_rep;  // 弾発射間隔

  uint8_t LLaserRef; // 太レーザーの参照カウント

  BulletCommand t_cmd;  // 弾発射用コマンド
  LaserCommand l_cmd; // レーザー発射用コマンド

  // --- メソッド ---
  void Draw() const;
  void UpdateAnimation();  // EnemyAnimeMove()
};

// 後方互換用エイリアス
// (ENEMY_DATA alias removed — use EnemyData directly)
// (INT_VECTOR alias removed — use InterruptVector directly)

struct ANIME_DATA {
  uint8_t mode;                 // アニメーションモード
  uint8_t n;                    // アニメーションパターン数
  PIXEL_SIZE size;              // 絵の幅, 絵の高さ
  PIXEL_LTRB ptn[ANIMEPTN_MAX]; // アニメーションの存在する矩形領域

  template <uint8_t Count>
  void SetSheet(PIXEL_POINT topleft, PIXEL_SIZE size, uint8_t mode) {
    static_assert(Count <= ANIMEPTN_MAX);

    this->size = size;
    this->n = Count;
    this->mode = mode;

    for (auto i = decltype(Count){0}; i < Count; i++) {
      ptn[i] = PIXEL_LTWH{topleft.x, topleft.y, size.w, size.h};
      topleft.x += size.w;
    }
  }

  template <uint8_t Count, PIXEL_COORD Size>
  void SetSheet(PIXEL_POINT topleft, uint8_t mode) {
    SetSheet<Count>(topleft, {Size, Size}, mode);
  }

  template <PIXEL_COORD Size> void SetSheetDeg(PIXEL_POINT topleft) {
    SetSheet<16>(topleft, {Size, Size}, ANM_DEG);
  }
};

//// 敵変数 ////
// Enemies.entities, Enemies.indices, Enemies.count, Enemies.anime で直接アクセス

//// 敵制御関数 ////
// 後方互換 inline wrapper は enemy_manager.h 末尾に移動
// 実装は EnemyManager メソッドに移行
