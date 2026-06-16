/*
 *   EnemyManager — centralized enemy system state and operations
 */

#pragma once

#include "ENEMY.h"
#include "platform/buffer.h"
#include <array>
#include <cstdint>

struct EnemyManager {
  // --- 敵データ ---
  std::array<EnemyData, ENEMY_MAX> entities;   // Enemy[]
  std::array<uint16_t, ENEMY_MAX> indices;      // EnemyInd[]
  uint16_t count = 0;                            // EnemyNow

  // --- ECL/SCL データ ---
  BYTE_BUFFER_OWNED ecl_head;   // ECL_Head
  BYTE_BUFFER_OWNED scl_head;   // SCL_Head
  uint8_t* scl_now = nullptr;   // SCL_Now

  // --- アニメーション ---
  ANIME_DATA anime[ANIME_MAX];   // Anime[]

  // --- ホーミング ---
  int homing_x = 0;              // HomingX
  int homing_y = 0;              // HomingY
  int homing_flag = 0;           // HomingFlag

  // --- 特殊角度（ENEMY.cpp 内で使用）---
  uint8_t enemy_exdeg = 0;       // EnemyEXDEG
  uint8_t enemy_exdeg_d = 0;     // EnemyEXDEG_D

  // === メソッド ===

  // ホーミング
  void UpdateHoming(const EnemyData *e);

  // ナナメレーザーヒットチェック（ユーティリティ）
  static bool LaserHITCHK(const EnemyData *e, int ox, int oy, uint8_t d);

  // 敵の移動・描画・管理
  void Move();          // was enemy_move
  void Draw();          // was enemy_draw
  void Clear();         // was enemy_clear
  void InitIndices();   // was enemyind_set

  // ダメージ
  bool ApplyDamage(EnemyData &e, int damage);          // was EnemyDamageApply
  bool DamageAt(int x, int y, int damage);             // was enemy_damage
  bool DamageAt2(int x, int y, int damage);            // was enemy_damage2
  void DamageAt3(int x, int y, uint8_t d);             // was enemy_damage3
  void DamageAll(int damage);                          // was enemy_damage4

  // 敵データ初期化
  void InitDataX64(EnemyData *e, int x, int y, uint32_t EclID);  // was InitEnemyDataX64
  void InitDataSTD(EnemyData *e, short x, short y, uint32_t EclID); // was InitEnemyDataSTD
  void ECL_LongJump(EnemyData *e, uint32_t EclID);         // was EnemyECL_LongJump

  // アニメーション
  void UpdateAnimation(EnemyData *e);                     // was EnemyAnimeMove

  // ECL
  void ParseECL(EnemyData *e);                            // was parse_ECL
  void CheckECLInterrupt(EnemyData *e);                   // was CheckECLInterrupt
  void InitECLInterrupt(EnemyData *e);                    // was InitECLInterrupt
};

extern EnemyManager Enemies;

// === 後方互換 inline wrapper ===
inline void UpdateHoming(const EnemyData *e) { Enemies.UpdateHoming(e); }
inline bool LaserHITCHK(const EnemyData *e, int ox, int oy, uint8_t d) { return EnemyManager::LaserHITCHK(e, ox, oy, d); }
inline void enemy_move(void) { Enemies.Move(); }
inline void enemy_draw(void) { Enemies.Draw(); }
inline void enemyind_set(void) { Enemies.InitIndices(); }
inline void enemy_clear(void) { Enemies.Clear(); }
inline bool enemy_damage(int x, int y, int damage) { return Enemies.DamageAt(x, y, damage); }
inline bool enemy_damage2(int x, int y, int damage) { return Enemies.DamageAt2(x, y, damage); }
inline void enemy_damage3(int x, int y, uint8_t d) { Enemies.DamageAt3(x, y, d); }
inline void enemy_damage4(int damage) { Enemies.DamageAll(damage); }
inline void InitEnemyDataX64(EnemyData *e, int x, int y, uint32_t EclID) { Enemies.InitDataX64(e, x, y, EclID); }
inline void InitEnemyDataSTD(EnemyData *e, short x, short y, uint32_t EclID) { Enemies.InitDataSTD(e, x, y, EclID); }
inline void EnemyECL_LongJump(EnemyData *e, uint32_t EclID) { Enemies.ECL_LongJump(e, EclID); }
inline void EnemyAnimeMove(EnemyData *e) { Enemies.UpdateAnimation(e); }
inline void parse_ECL(EnemyData *e) { Enemies.ParseECL(e); }
inline void CheckECLInterrupt(EnemyData *e) { Enemies.CheckECLInterrupt(e); }
inline void InitECLInterrupt(EnemyData *e) { Enemies.InitECLInterrupt(e); }
