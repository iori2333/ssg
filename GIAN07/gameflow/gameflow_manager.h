///
/// GameFlowManager - Centralized game flow state
///

#pragma once

#include <array>
#include <cstdint>
#include <functional>

#include "game_main.h"
#include "score.h"

#include "core/game_context.h"

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
  ScoreName,
  NameRegist,
  Ending,
  SProject,
  ReplayAll,
  MusicRoom,
  External, // External specification via GameInit(), etc.
  BulletGallery, // Debug bullet gallery
};

struct GameFlowManager {
  std::function<void(bool &)> game_main;
  GameState current_state = GameState::Title;
  uint16_t demo_timer = 0;
  uint32_t draw_count = 0;
  uint8_t weapon_key_wait = 0;
  int game_over_timer = 0;
  NrNameData current_name = {};
  uint8_t current_rank = 0;
  uint8_t current_dif = 0;
  bool input_locked = false;

  GameContext ctx{};

  // === Methods ===
  void TitleProc(bool &);
  void ScoreNameProc(bool &);
  static void ScoreDraw();
  void NameRegistProc(bool &);
  bool NameRegistInit(bool bNeedChgMusic = false);
  bool WeaponSelectInit(bool ExStg);
  void WeaponSelectProc(bool &);
  void GameOverProc0(bool &);
  bool IsDraw();
  static char GetAddr2Char(int x, int y);
};

extern GameFlowManager GameFlow;
