///
/// PlayerManager - Centralized player system state and operations
///

#pragma once

#include "player_shot.h"
#include "player_types.h"
#include <array>
#include <cstdint>

struct PlayerManager {
  Player viv{};                                       // Viv
  std::array<Bullet, MAIDTAMA_MAX> maid_tama{};       // MaidTama[]
  std::array<uint16_t, MAIDTAMA_MAX> maid_tama_ind{}; // MaidTamaInd[]
  uint16_t maid_tama_now = 0;                         // MaidTamaNow

  // === Methods ===
  void SetMaidShot();
  void MoveMaidShot();
  void DrawMaidShot();
  void SetMaidShotIndices();
  static void SetMLaser(uint16_t time);
};

extern PlayerManager Players;
