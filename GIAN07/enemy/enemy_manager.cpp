/*
 *   EnemyManager — centralized enemy system state
 */

#include "enemy_manager.h"

// --- グローバルインスタンス ---
EnemyManager Enemies;

// 後方互換用参照ラッパー (未移行分のみ保持)
// Enemy, EnemyInd, EnemyNow → Enemies.* 移行待ち (SCROLL.cpp, EnemyExCtrl.cpp)
std::array<EnemyData, ENEMY_MAX>& Enemy = Enemies.entities;
std::array<uint16_t, ENEMY_MAX>& EnemyInd = Enemies.indices;
uint16_t& EnemyNow = Enemies.count;
// Anime → Enemies.anime 移行待ち (LOADER.cpp 294 references)
ANIME_DATA (&Anime)[ANIME_MAX] = Enemies.anime;
