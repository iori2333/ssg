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

// ============================================================
// Backward-compat inline wrappers
// ============================================================
inline void tama_set()        { Bullets.Spawn(); }
inline void tama_setEX()      { Bullets.SpawnEX(); }
inline void tama_setLine()    { Bullets.SpawnLine(); }
inline void tama_setExtra01() { Bullets.SpawnExtra01(); }
inline int  TamaSpeedEx(uint8_t d) { return Bullets.SpeedEx(d); }
inline void tama_move()       { Bullets.Move(); }
inline void tama_draw()       { Bullets.Draw(); }
inline void tama_clear()      { Bullets.Clear(); }
inline uint32_t tama2score()  { return Bullets.ScoreToItems(); }
inline void tama2item(uint8_t n) { Bullets.ToItems(n); }
inline void tamaind_set(uint16_t t1) { Bullets.SetIndices(t1); }
inline uint8_t tama_dir(uint16_t i)  { return Bullets.Dir(i); }
inline int  NewTamaSpeed(uint16_t i)  { return Bullets.NewSpeed(i); }
inline int  LineCmdNewTamaSpeed(uint16_t i) { return Bullets.LineCmdNewSpeed(i); }
inline int  tama_speed(uint16_t i)    { return Bullets.Speed(i); }
inline uint8_t tama_flag()    { return Bullets.Flag(); }
inline void tamaTmove(TAMA_DATA* t) { Bullets.MoveByType(t); }
inline void tamaOmove(TAMA_DATA* t) { Bullets.MoveByOption(t); }
inline void tamaEmove(TAMA_DATA* t) { Bullets.MoveByEffect(t); }
