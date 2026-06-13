/*
 *   BossManager — centralized boss system state
 */

#include "boss_manager.h"

// --- グローバルインスタンス ---
BossManager Bosses;

// --- 後方互換用参照ラッパー ---
// クロスモジュール参照
std::array<BOSS_DATA, BOSS_MAX>& Boss = Bosses.bosses;
uint16_t& BossNow = Bosses.count;
// BossHPG → Bosses.hpg で直接アクセス
