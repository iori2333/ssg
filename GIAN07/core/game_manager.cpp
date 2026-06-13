/*
 *   GameManager — centralized game state
 */

#include "game_manager.h"

// --- グローバルインスタンス ---
GameManager GameState;

// --- 後方互換用参照ラッパー ---
uint32_t& GameCount = GameState.game_count;
uint8_t& GameStage = GameState.game_stage;
uint8_t& GameLevel = GameState.game_level;
bool& IsDemoplay = GameState.is_demoplay;
