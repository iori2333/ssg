/*
 *   GameFlowManager — centralized game flow state
 */

#include "gameflow_manager.h"

// --- グローバルインスタンス ---
GameFlowManager GameFlow;

// --- 後方互換用参照ラッパー ---
// GameMain のみクロスモジュールのため保持（ENTRY.cpp, MUSIC.cpp で使用）
void (*& GameMain)(bool &quit) = GameFlow.game_main;
// その他（DemoTimer, DrawCount, WeaponKeyWait, GameOverTimer, CurrentName,
// CurrentRank, CurrentDif, VivTemp, InputLocked, FlashState, ScoreString）
// → 各 .cpp は GameFlow.xxx を直接使用
