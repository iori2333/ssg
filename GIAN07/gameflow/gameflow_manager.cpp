/*
 *   GameFlowManager — centralized game flow state
 */

#include "gameflow_manager.h"

// --- グローバルインスタンス ---
GameFlowManager GameFlow;

// --- 後方互換用参照ラッパー ---
// GAMEMAIN
void (*& GameMain)(bool &quit) = GameFlow.game_main;
uint16_t& DemoTimer = GameFlow.demo_timer;
uint32_t& DrawCount = GameFlow.draw_count;
uint8_t&  WeaponKeyWait = GameFlow.weapon_key_wait;
int&      GameOverTimer = GameFlow.game_over_timer;
NR_NAME_DATA& CurrentName = GameFlow.current_name;
uint8_t&  CurrentRank = GameFlow.current_rank;
uint8_t&  CurrentDif = GameFlow.current_dif;
MAID& VivTemp = GameFlow.viv_temp;
bool& InputLocked = GameFlow.input_locked;

// ENDING
uint16_t& FlashState = GameFlow.flash_state;

// SCORE
std::array<NR_SCORE_STRING, NR_RANK_MAX>& ScoreString = GameFlow.score_string;
