/*                                                                           */
/*   MaidTama.h   メイドさんなショットの処理                                 */
/*                                                                           */
/*                                                                           */

#pragma once

#include "bullet/bullet.h"

///// [ 定数 ] /////

// 最大値 //
inline constexpr auto MAIDTAMA_MAX = 200; // 自機ショットの最大数

inline constexpr auto TID_WIDE_MAIN = 0x00; // ワイド・メインショットのＩＤ
inline constexpr auto TID_WIDE_SUB = 0x01;  // ワイド・サブショットのＩＤ
inline constexpr auto TID_HOMING_MAIN =
    0x02;                                    // ホーミング・メインショットのＩＤ
inline constexpr auto TID_HOMING_SUB = 0x03; // ホーミング・サブショットのＩＤ
inline constexpr auto TID_LASER_MAIN =
    0x04; // レーザー・メインショット？？のＩＤ
inline constexpr auto TID_LASER_SUB = 0x05; // レーザー・サブショットのＩＤ

inline constexpr auto TID_HOMING_BOMB_A = 0x06; // ホーミング用ボム(移動中)
inline constexpr auto TID_HOMING_BOMB_B = 0x07; // ホーミング用ボム(誘爆中)

inline constexpr auto TDM_WIDE_MAIN = 6; // ワイド・メインショットのダメージ
inline constexpr auto TDM_WIDE_SUB = 4;  // ワイド・サブショットのダメージ
inline constexpr auto TDM_HOMING_MAIN =
    6; // ホーミング・メインショットのダメージ
inline constexpr auto TDM_HOMING_SUB = 7; // ホーミング・サブショットのダメージ
inline constexpr auto TDM_LASER_MAIN = 2; // レーザー・メインショットのダメージ
inline constexpr auto TDM_LASER_SUB = 5;  // レーザー・サブショットのダメージ

///// [ 関数 ] /////
// 後方互換 inline wrapper は player_manager.h 末尾に移動
// 実装は PlayerManager メソッドに移行

///// [ 変数 ] /////
// Players.maid_tama, Players.maid_tama_ind, Players.maid_tama_now
// で直接アクセス
