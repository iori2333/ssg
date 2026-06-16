/*
 *   GameFlowManager — centralized game flow state
 */

#pragma once

#include "GAMEMAIN.h"
#include "SCORE.h"
#include "player/player_types.h"
#include <array>
#include <cstdint>
#include <functional>

///// [ ゲーム状態列挙 ] /////
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
  External,   // GameInit() 等での外部指定
};

struct GameFlowManager {
  std::function<void(bool&)> game_main;
  GameState current_state = GameState::Title;
  uint16_t demo_timer = 0;
  uint32_t draw_count = 0;
  uint8_t weapon_key_wait = 0;
  int game_over_timer = 0;
  NR_NAME_DATA current_name = {};
  uint8_t current_rank = 0;
  uint8_t current_dif = 0;
  Player viv_temp = {};
  bool input_locked = false;

  // === メソッド ===
  void TitleProc(bool &);
  void ScoreNameProc(bool &);
  void ScoreDraw();
  void NameRegistProc(bool &);
  bool NameRegistInit(bool bNeedChgMusic = false);
  bool WeaponSelectInit(bool ExStg);
  void WeaponSelectProc(bool &);
  void GameOverProc0(bool &);
  bool IsDraw();
  static char GetAddr2Char(int x, int y);
};

extern GameFlowManager GameFlow;
