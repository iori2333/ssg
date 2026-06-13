/*
 *   BossManager — centralized boss system state
 */

#include "boss_manager.h"

// --- グローバルインスタンス ---
BossManager Bosses;

// --- 後方互換用参照ラッパー ---
std::array<BOSS_DATA, BOSS_MAX>& Boss = Bosses.bosses;
uint16_t& BossNow = Bosses.count;
BOSSHPG_INFO& BossHPG = Bosses.hpg;
