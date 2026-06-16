/*
 *   BossManager — centralized boss system state and operations
 */

#pragma once

#include "BOSS.h"
#include "EnemyExCtrl.h"
#include <array>
#include <cstdint>

struct BossManager {
  std::array<BossData, BOSS_MAX> bosses;     // Boss[]
  uint16_t count = 0;                          // BossNow
  BossHpgInfo hpg;                            // BossHPG

  // Snaky/Bit データ（旧 EnemyExCtrl.cpp ファイル静的変数）
  SNAKYMOVE_DATA<30> snake_data[SNAKE_MAX];   // SnakeData[]
  BitData bit_data;                           // BitData

  // === メソッド ===

  // 初期化・セット
  void Init();
  void Set(int x, int y, uint32_t BossID);
  void SetEx(int x, int y, uint32_t BossID);

  // 移動・描画
  void Move();
  void Draw();
  void ClearCmd();
  void DrawHPG();

  // 体力
  void KillAll();
  uint32_t GetHPSum();

  // ダメージ
  bool ApplyDamage(BossData &b, EnemyData &e, int damage);
  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t d);
  void DamageAll(int damage);

  // 割り込み・ビット制御
  void Interrupt(ENEMY_DATA *e, uint8_t IntID);
  void BitAttack(ENEMY_DATA *e, uint32_t AtkID);
  void BitLaser(ENEMY_DATA *e, uint8_t cmd);
  void BitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param);
  int GetBitLeft();

  // 蛇型の敵 (was in EnemyExCtrl.cpp)
  void SnakyInit();
  void SnakySet(BossData *b, int len, uint32_t TailID);
  void SnakyMove();
  void SnakyDelete(const BossData *b);

  // ビット (was in EnemyExCtrl.cpp)
  void BitInit();
  void BitSet(BossData *b, uint8_t NumBits, uint32_t BitID);
  void BitMove();
  void BitDelete();
  void BitLineDraw();
  void BitSelectAttack(uint32_t BitID);
  void BitLaserCommand(uint8_t Command);
  void BitSendCommand(uint8_t Command, int Param);
  int BitGetNum();

private:
  void HPG_Open(uint32_t max);
  void HPG_Move(uint32_t now);
  void HPG_Close();
  void HPG_Update(uint32_t next);
  int PutBoss(int x, int y, uint32_t id);
  static void STDMove(BossData *b);
  void BitSTDRoll();
  void BitSTDRad();
};

extern BossManager Bosses;
