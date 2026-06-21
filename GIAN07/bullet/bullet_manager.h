///
/// BulletManager - Centralized bullet system state
///

#pragma once

#include <array>
#include <cstdint>

#include "bullet.h"

struct BulletManager {
  // --- Bullet data ---
  std::array<Bullet, TAMA_MAX> bullets;         // Tama[]
  BulletCommand command;                        // TamaCmd
  std::array<uint16_t, TAMA_MAX> indices_small; // Tama1Ind[]
  std::array<uint16_t, TAMA_MAX> indices_large; // Tama2Ind[]
  uint16_t count_small = 0;                     // Tama1Now
  uint16_t count_large = 0;                     // Tama2Now
  uint16_t max_small = 0;                       // Tama1Max
  uint16_t max_large = 0;                       // Tama2Max
  int speed = 0;                                // TamaSpeed

  // --- Public methods ---
  void Spawn();
  void SpawnEX();
  void SpawnLine();
  void SpawnExtra01();
  int SpeedEx(uint8_t d) const;
  void Move();
  void Draw();
  void Clear();
  uint32_t ScoreToItems();
  void ToItems(uint8_t n);
  void SetIndices(uint16_t tama1);
  uint8_t Dir(uint16_t i) const;
  int NewSpeed(uint16_t i) const;
  int LineCmdNewSpeed(uint16_t i) const;
  int Speed(uint16_t i) const;
  uint8_t Flag() const;
  static void MoveByType(Bullet *t);
  void MoveByOption(Bullet *t);
  static void MoveByEffect(Bullet *t);

private:
  void TamaSetMain();
  void SetEasy();
  void SetHard();
  void SetLunatic();
};

extern BulletManager Bullets;

//// Bullet command macros (moved from TAMA.h -- reference Bullets.command
///directly) ////

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
