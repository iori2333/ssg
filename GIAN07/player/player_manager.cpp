/*
 *   PlayerManager — centralized player system state
 */

#include "player_manager.h"

// --- グローバルインスタンス ---
PlayerManager Players;

// --- 後方互換用参照ラッパー ---
// クロスモジュール参照
Player& Viv = Players.viv;
uint16_t& MaidTamaNow = Players.maid_tama_now; // GIAN.cpp debug 用
// MaidTama, MaidTamaInd → Players.xxx で直接アクセス
