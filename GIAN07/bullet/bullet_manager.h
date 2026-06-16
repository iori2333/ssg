/*
 *   BulletManager — centralized bullet system state
 */

#pragma once

#include "TAMA.h"
#include <array>
#include <cstdint>

struct BulletManager {
  // --- 弾データ ---
  std::array<Bullet, TAMA_MAX> bullets;           // Tama[]
  BulletCommand command;                           // TamaCmd
  std::array<uint16_t, TAMA_MAX> indices_small;    // Tama1Ind[]
  std::array<uint16_t, TAMA_MAX> indices_large;    // Tama2Ind[]
  uint16_t count_small = 0;                        // Tama1Now
  uint16_t count_large = 0;                        // Tama2Now
  uint16_t max_small = 0;                          // Tama1Max
  uint16_t max_large = 0;                          // Tama2Max
  int speed = 0;                                    // TamaSpeed

  // --- Public methods ---
  void Spawn();                    // was tama_set
  void SpawnEX();                  // was tama_setEX
  void SpawnLine();                // was tama_setLine
  void SpawnExtra01();             // was tama_setExtra01
  int SpeedEx(uint8_t d);          // was TamaSpeedEx
  void Move();                     // was tama_move
  void Draw();                     // was tama_draw
  void Clear();                    // was tama_clear
  uint32_t ScoreToItems();         // was tama2score
  void ToItems(uint8_t n);         // was tama2item
  void SetIndices(uint16_t count1); // was tamaind_set
  uint8_t Dir(uint16_t i);         // was tama_dir
  int NewSpeed(uint16_t i);        // was NewTamaSpeed
  int LineCmdNewSpeed(uint16_t i); // was LineCmdNewTamaSpeed
  int Speed(uint16_t i);           // was tama_speed
  uint8_t Flag();                  // was tama_flag
  void MoveByType(TAMA_DATA* t);   // was tamaTmove
  void MoveByOption(TAMA_DATA* t); // was tamaOmove
  void MoveByEffect(TAMA_DATA* t); // was tamaEmove

private:
  void TamaSetMain();              // was __TamaSet
  void SetEasy();                  // was easy_cmd
  void SetHard();                  // was hard_cmd
  void SetLunatic();               // was luna_cmd
};

extern BulletManager Bullets;

//// 弾コマンド用マクロ (TAMA.h から移動 — Bullets.command を直接参照) ////

inline void TamaSetForm(uint8_t cmd, uint8_t option, uint8_t type, uint8_t c) {
  Bullets.command.cmd = cmd;
  Bullets.command.option = option;
  Bullets.command.type = type;
  Bullets.command.c = c;
}

inline void TamaSTDForm(uint8_t c) { TamaSetForm(TC_WAY, TOP_NONE, T_NORM, c); }

inline void TamaSetDeg(uint8_t d, uint8_t dw) {
  Bullets.command.d = d;
  Bullets.command.dw = dw;
}

inline void TamaSetNum(uint8_t n, uint8_t ns) {
  Bullets.command.n = n;
  Bullets.command.ns = ns;
}

inline void TamaSetSpd(uint8_t v, char a) {
  Bullets.command.v = v;
  Bullets.command.a = a;
}

inline void TamaSetXY(int x, int y) {
  Bullets.command.x = x;
  Bullets.command.y = y;
}

