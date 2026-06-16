/*
 *   PlayerManager — centralized player system state and operations
 */

#pragma once

#include "MAID.h"
#include "MAIDTAMA.h"
#include <array>
#include <cstdint>

struct PlayerManager {
  Player viv;                                               // Viv
  std::array<TAMA_DATA, MAIDTAMA_MAX> maid_tama;            // MaidTama[]
  std::array<uint16_t, MAIDTAMA_MAX> maid_tama_ind;         // MaidTamaInd[]
  uint16_t maid_tama_now = 0;                               // MaidTamaNow

  // === メソッド ===
  void SetMaidShot();             // was MaidTamaSet
  void MoveMaidShot();            // was MaidTamaMove
  void DrawMaidShot();            // was MaidTamaDraw
  void SetMaidShotIndices();      // was MaidTamaIndSet
  void SetMLaser(uint16_t time);  // was MLaserSet
};

extern PlayerManager Players;

// === 後方互換 inline wrapper ===
inline void MaidTamaSet(void) { Players.SetMaidShot(); }
inline void MaidTamaMove(void) { Players.MoveMaidShot(); }
inline void MaidTamaDraw(void) { Players.DrawMaidShot(); }
inline void MaidTamaIndSet(void) { Players.SetMaidShotIndices(); }
inline void MLaserSet(uint16_t time) { Players.SetMLaser(time); }
