/*
 *   GameFlowManager — centralized game flow state
 */

#pragma once

#include "GAMEMAIN.h"
#include "MAID.h"
#include "SCORE.h"
#include <array>
#include <cstdint>

struct GameFlowManager {
  // GAMEMAIN globals
  void (*game_main)(bool &quit) = nullptr;
  uint16_t demo_timer = 0;
  uint32_t draw_count = 0;
  uint8_t weapon_key_wait = 0;
  int game_over_timer = 0;
  NR_NAME_DATA current_name = {};
  uint8_t current_rank = 0;
  uint8_t current_dif = 0;
  MAID viv_temp = {};
  bool input_locked = false;

  // ENDING globals
  uint16_t flash_state = 0;

  // SCORE globals
  std::array<NR_SCORE_STRING, NR_RANK_MAX> score_string = {};
};

extern GameFlowManager GameFlow;

// --- 後方互換用参照 ---
extern void (*& GameMain)(bool &quit);
extern uint16_t& DemoTimer;
extern uint32_t& DrawCount;
extern uint8_t& WeaponKeyWait;
extern int& GameOverTimer;
extern NR_NAME_DATA& CurrentName;
extern uint8_t& CurrentRank;
extern uint8_t& CurrentDif;
extern MAID& VivTemp;
extern bool& InputLocked;
extern uint16_t& FlashState; // from ENDING.cpp
