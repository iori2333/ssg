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
  void Move();
  void Draw();
  void Clear();
  void InitIndices();

  // ダメージ
  bool ApplyDamage(EnemyData &e, int damage);
  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t d);
  void DamageAll(int damage);

  // 敵データ初期化
  void InitDataX64(EnemyData *e, int x, int y, uint32_t EclID);
  void InitDataSTD(EnemyData *e, short x, short y, uint32_t EclID);
  void ECL_LongJump(EnemyData *e, uint32_t EclID);

  // アニメーション
  void UpdateAnimation(EnemyData *e);

  // ECL
  void ParseECL(EnemyData *e);
  void CheckECLInterrupt(EnemyData *e);
  void InitECLInterrupt(EnemyData *e);
};

extern EnemyManager Enemies;
