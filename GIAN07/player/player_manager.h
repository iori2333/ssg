/*
 *   PlayerManager — centralized player system state
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
};

extern PlayerManager Players;
// 後方互換用参照は MAID.h, MAIDTAMA.h で宣言
