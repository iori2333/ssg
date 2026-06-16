/*
 *   GameFlowManager — centralized game flow state
 */

#pragma once

#include "GAMEMAIN.h"
#include "MAID.h"
#include "SCORE.h"
#include <array>
#include <cstdint>
#include <functional>

struct GameFlowManager {
  // GAMEMAIN globals
  std::function<void(bool&)> game_main;  // was void (*)(bool&)
  uint16_t demo_timer = 0;
  uint32_t draw_count = 0;
  uint8_t weapon_key_wait = 0;
  int game_over_timer = 0;
  NR_NAME_DATA current_name = {};
  uint8_t current_rank = 0;
  uint8_t current_dif = 0;
  MAID viv_temp = {};
  bool input_locked = false;

  // ENDING globals → ending_manager.h の EndingManager に移動

  // SCORE globals → score_manager.h の ScoreManager に移動

  // === メソッド ===
  void ScoreNameProc(bool &);
  void ScoreDraw();
  void NameRegistProc(bool &);
  bool NameRegistInit(bool bNeedChgMusic = false);
  bool WeaponSelectInit(bool ExStg);
  void WeaponSelectProc(bool &);
  void GameOverProc0(bool &);
  void TitleProc(bool &);
  bool IsDraw();
  static char GetAddr2Char(int x, int y);
};

extern GameFlowManager GameFlow;

// --- 後方互換用参照 ---
// GameMain のみクロスモジュール（ENTRY.cpp, MUSIC.cpp で使用）
extern std::function<void(bool&)>& GameMain;
// その他の参照は削除 — 各 .cpp は GameFlow.xxx を直接使用

// GameMain が特定の関数ポインタを指しているか比較するヘルパー
// std::function は生の関数ポインタと直接比較できないため
inline bool GameMainIs(void(*fp)(bool&)) {
  auto *p = GameMain.target<void(*)(bool&)>();
  return p && *p == fp;
}

// === 後方互換 inline wrapper (converted methods) ===
inline void TitleProc(bool &q) { GameFlow.TitleProc(q); }
inline void ScoreNameProc(bool &q) { GameFlow.ScoreNameProc(q); }
inline void ScoreDraw() { GameFlow.ScoreDraw(); }
inline void NameRegistProc(bool &q) { GameFlow.NameRegistProc(q); }
inline bool NameRegistInit(bool b = false) { return GameFlow.NameRegistInit(b); }
inline bool WeaponSelectInit(bool ExStg) { return GameFlow.WeaponSelectInit(ExStg); }
inline void WeaponSelectProc(bool &q) { GameFlow.WeaponSelectProc(q); }
inline void GameOverProc0(bool &q) { GameFlow.GameOverProc0(q); }
inline bool IsDraw() { return GameFlow.IsDraw(); }
inline char GetAddr2Char(int x, int y) { return GameFlowManager::GetAddr2Char(x, y); }
