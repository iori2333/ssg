/*
 *   PlayerManager — centralized player system state
 */

#include "player_manager.h"

// --- グローバルインスタンス ---
PlayerManager Players;

// --- 後方互換用参照ラッパー ---
Player& Viv = Players.viv;
std::array<TAMA_DATA, MAIDTAMA_MAX>& MaidTama = Players.maid_tama;
std::array<uint16_t, MAIDTAMA_MAX>& MaidTamaInd = Players.maid_tama_ind;
uint16_t& MaidTamaNow = Players.maid_tama_now;
