/*
 *   PlayerManager — centralized player system state and operations
 */

#pragma once

#include "player_types.h"
#include "MAIDTAMA.h"
#include <array>
#include <cstdint>

struct PlayerManager {
  Player viv;                                               // Viv
  std::array<Bullet, MAIDTAMA_MAX> maid_tama;            // MaidTama[]
  std::array<uint16_t, MAIDTAMA_MAX> maid_tama_ind;         // MaidTamaInd[]
  uint16_t maid_tama_now = 0;                               // MaidTamaNow

  // === メソッド ===
  void SetMaidShot();
  void MoveMaidShot();
  void DrawMaidShot();
  void SetMaidShotIndices();
  void SetMLaser(uint16_t time);
};

extern PlayerManager Players;
