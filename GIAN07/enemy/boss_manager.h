///
/// BossManager - centralized boss system state and operations
///

#pragma once

#include <array>
#include <cstdint>

#include "boss.h"
#include "boss_systems.h"

struct BossManager {
  std::array<BossData, BOSS_MAX> bosses; // Boss[]
  uint16_t count = 0;                    // BossNow
  BossHpgInfo hpg;                       // BossHPG

  // Snaky/Bit data (formerly file-static in EnemyExCtrl.cpp)
  SNAKYMOVE_DATA<30> snake_data[SNAKE_MAX]; // SnakeData[]
  BitData bit_data;                         // BitData

  // === Methods ===

  // Initialization and setup
  void Init();
  void Set(int x, int y, uint32_t BossID);
  void SetEx(int x, int y, uint32_t BossID);

  // Movement and drawing
  void Move();
  void Draw();
  void ClearCmd();
  void DrawHPG();

  // Timer
  void SetSCLTimeout(int32_t timeout_end);

  // HP
  void KillAll();
  uint32_t GetHPSum();

  // Damage
  bool ApplyDamage(BossData &b, EnemyData &e, int damage);
  bool DamageAt(int x, int y, int damage);
  bool DamageAt2(int x, int y, int damage);
  void DamageAt3(int x, int y, uint8_t d);
  void DamageAll(int damage);

  // Interrupts and bit control
  void Interrupt(EnemyData *e, uint8_t IntID);
  void BitAttack(EnemyData *e, uint32_t AtkID);
  void BitLaser(EnemyData *e, uint8_t cmd);
  void BitCommand(EnemyData *e, uint8_t Cmd, int Param);
  int GetBitLeft() const;

  // Snake-type enemy (was in EnemyExCtrl.cpp)
  void SnakyInit();
  void SnakySet(BossData *b, int len, uint32_t TailID);
  void SnakyMove();
  void SnakyDelete(const BossData *b);

  // Bit (was in EnemyExCtrl.cpp)
  void BitInit();
  void BitSet(BossData *b, uint8_t NumBits, uint32_t BitID);
  void BitMove();
  void BitDelete();
  void BitLineDraw();
  void BitSelectAttack(uint32_t BitID);
  void BitLaserCommand(uint8_t Command);
  void BitSendCommand(uint8_t Command, int Param);
  int BitGetNum() const;

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
