///
/// GameFlowManager - Centralized game flow state
///

#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "game_context.h"
#include "game_main.h"

///// [ Game state enum ] /////
enum class GameState {
  Title,
  WeaponSelect,
  Game,
  Pause,
  Demo,
  GameOver0,
  GameOverSave,
  GameOver,
  Leaderboard,
  NameRegistration,
  Ending,
  SProject,
  ReplayAll,
  MusicRoom,
  External,      // External specification via GameInit(), etc.
  BulletGallery, // Debug bullet gallery
};

struct GameFlowManager {
  std::function<void(bool &)> game_main;
  GameState current_state = GameState::Title;
  uint16_t demo_timer = 0;
  uint32_t draw_count = 0;
  uint8_t weapon_key_wait = 0;
  int game_over_timer = 0;

  GameContext ctx{};

  // === Methods ===
  void TitleProc(bool &);
  bool WeaponSelectInit(bool ExStg);
  void WeaponSelectProc(bool &);
  void GameOverProc0(bool &);
  bool IsDraw();
};

extern GameFlowManager GameFlow;
