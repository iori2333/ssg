/*                                                                           */
/*   Boss.h   ボスの処理(中ボス含む)                                         */
/*                                                                           */
/*                                                                           */

#pragma once

#include "enemy.h"
#include <array>
#include <cstdint>

///// [構造体] /////

// 特殊当たり判定 //
struct ExHitCheck {
  uint8_t flags[60][60];
};
// (EXHITCHK alias removed — use ExHitCheck directly)

// ボスデータ //
struct BossData {
  EnemyData Edat; // 標準の敵データ(実体であることに注意)
  ExHitCheck *Hit;   // 特殊当たり判定(NULL なら使用しない)

  void (*ExMove)(BossData *); // 特殊移動用関数

  uint32_t ExCount; // ある状態におけるカウンタ(推移時にゼロ初期化)
  uint8_t ExState;  // 特殊状態
  bool IsUsed = false;   // このデータは使用されているか
};
// (BOSS_DATA alias removed — use BossData directly)

///// [ 定数 ] /////
inline constexpr auto BOSS_MAX = 4; // ボスの最大出現数
inline constexpr auto BOSSHPG_HEIGHT = 24; // 体力ゲージの高さ

// ボスの体力ゲージ //
struct BossHpgInfo {
  uint32_t Now, Max; // 体力の現在値＆最大値
  uint32_t Next;     // 次の体力の値
  uint32_t Update;   // 更新用の値
  uint32_t Count;    // フレーム数保持

  uint16_t XTemp[BOSSHPG_HEIGHT]; // ＨＰゲージの演出用
  uint8_t State;                  // 状態
};

///// [ 関数 ] /////
// 後方互換 inline wrapper は boss_manager.h 末尾に移動
// 実装は BossManager メソッドに移行

///// [ 変数 ] /////
// Bosses.bosses, Bosses.count, Bosses.hpg で直接アクセス
