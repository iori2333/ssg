/*                                                                           */
/*   Item.h   アイテムの処理                                                 */
/*                                                                           */
/*                                                                           */

#pragma once

#include <array>
#include <cstdint>

///// [ 定数 ] /////

// 最大数 //
inline constexpr auto ITEM_MAX = 100;

// 種類もしくは状態 //
inline constexpr auto ITEM_DELETE = 0x00; // 消去要請
inline constexpr auto ITEM_SCORE = 0x01;  // 得点アイテム
inline constexpr auto ITEM_EXTEND = 0x02; // 残りメイド数ＵＰ
inline constexpr auto ITEM_BOMB = 0x03;   // ボム

// その他 //
inline constexpr auto ITEM_GRAVITY = 3;          // アイテムに対するｙ加速度
inline constexpr auto ITEM_HITX = (8 + 8) * 64;  // アイテムのＸ当たり判定
inline constexpr auto ITEM_HITY = (16 + 8) * 64; // アイテムのＹ当たり判定

///// [構造体] /////
struct ItemData {
  int x, y;
  int vx, vy;
  uint32_t count;
  uint8_t type;
  bool auto_collect; // 自動回収が発動済みか
};
using ITEM_DATA = ItemData;

///// [ 関数 ] /////
void ItemSet(int x, int y, uint8_t type); // アイテムを発生させる
void ItemMove(void);                      // アイテムを動かす
void ItemDraw(void);                      // アイテムを描画する

void ItemIndSet(void); // アイテム配列の初期化

///// [ 変数 ] /////
extern std::array<ItemData, ITEM_MAX> &Item;
extern std::array<uint16_t, ITEM_MAX> &ItemInd;
extern uint16_t &ItemNow;
