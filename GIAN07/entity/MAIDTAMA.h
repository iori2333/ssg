/*                                                                           */
/*   MaidTama.h   メイドさんなショットの処理                                 */
/*                                                                           */
/*                                                                           */

#pragma once

#include "entity/TAMA.h"

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
void MaidTamaSet(void);    // たま発射！！
void MaidTamaMove(void);   // 弾移動＆ヒットチェック
void MaidTamaDraw(void);   // ナニな弾描画
void MaidTamaIndSet(void); // 弾ハッシュテーブル初期化

///// [ 変数 ] /////
extern std::array<TAMA_DATA, MAIDTAMA_MAX>
    MaidTama; // 自機ショットの格納用構造体
extern std::array<uint16_t, MAIDTAMA_MAX>
    MaidTamaInd;             // 弾の順番を維持するための配列(TAMA.CPP互換)
extern uint16_t MaidTamaNow; // 現在の数
