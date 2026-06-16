/*
 *   BossManager — centralized boss system state and operations
 */

#pragma once

#include "BOSS.h"
#include "EnemyExCtrl.h"
#include <array>
#include <cstdint>

struct BossManager {
  std::array<BOSS_DATA, BOSS_MAX> bosses;     // Boss[]
  uint16_t count = 0;                          // BossNow
  BOSSHPG_INFO hpg;                            // BossHPG

  // Snaky/Bit データ（旧 EnemyExCtrl.cpp ファイル静的変数）
  SNAKYMOVE_DATA<30> snake_data[SNAKE_MAX];   // SnakeData[]
  BIT_DATA bit_data;                           // BitData

  // === メソッド ===

  // 初期化・セット
  void Init();                                              // was BossDataInit
  void Set(int x, int y, uint32_t BossID);                 // was BossSet
  void SetEx(int x, int y, uint32_t BossID);               // was BossSetEx

  // 移動・描画
  void Move();                                              // was BossMove
  void Draw();                                              // was BossDraw
  void ClearCmd();                                          // was BossClearCmd
  void DrawHPG();                                           // was BossHPG_Draw

  // 体力
  void KillAll();                                           // was BossKillAll
  uint32_t GetHPSum();                                      // was GetBossHPSum

  // ダメージ
  bool ApplyDamage(BOSS_DATA &b, ENEMY_DATA &e, int damage); // was BossDamageApply
  bool DamageAt(int x, int y, int damage);                   // was BossDamage
  bool DamageAt2(int x, int y, int damage);                  // was BossDamage2
  void DamageAt3(int x, int y, uint8_t d);                   // was BossDamage3
  void DamageAll(int damage);                                // was BossDamage4

  // 割り込み・ビット制御
  void Interrupt(ENEMY_DATA *e, uint8_t IntID);              // was BossINT
  void BitAttack(ENEMY_DATA *e, uint32_t AtkID);             // was BossBitAttack
  void BitLaser(ENEMY_DATA *e, uint8_t cmd);                 // was BossBitLaser
  void BitCommand(ENEMY_DATA *e, uint8_t Cmd, int Param);    // was BossBitCommand
  int GetBitLeft();                                          // was BossGetBitLeft

  // 蛇型の敵 (was in EnemyExCtrl.cpp)
  void SnakyInit();
  void SnakySet(BOSS_DATA *b, int len, uint32_t TailID);
  void SnakyMove();
  void SnakyDelete(const BOSS_DATA *b);

  // ビット (was in EnemyExCtrl.cpp)
  void BitInit();
  void BitSet(BOSS_DATA *b, uint8_t NumBits, uint32_t BitID);
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
  static void STDMove(BOSS_DATA *b);
  void BitSTDRoll();
  void BitSTDRad();
};

extern BossManager Bosses;
