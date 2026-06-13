/*
 *   EnemyManager — centralized enemy system state
 */

#include "enemy_manager.h"

// --- グローバルインスタンス ---
EnemyManager Enemies;

// --- 後方互換用参照ラッパー ---
std::array<EnemyData, ENEMY_MAX>& Enemy = Enemies.entities;
std::array<uint16_t, ENEMY_MAX>& EnemyInd = Enemies.indices;
uint16_t& EnemyNow = Enemies.count;
BYTE_BUFFER_OWNED& ECL_Head = Enemies.ecl_head;
BYTE_BUFFER_OWNED& SCL_Head = Enemies.scl_head;
uint8_t*& SCL_Now = Enemies.scl_now;
ANIME_DATA (&Anime)[ANIME_MAX] = Enemies.anime;
int& HomingX = Enemies.homing_x;
int& HomingY = Enemies.homing_y;
int& HomingFlag = Enemies.homing_flag;

// --- 特殊角度 ---
uint8_t& EnemyEXDEG = Enemies.enemy_exdeg;
uint8_t& EnemyEXDEG_D = Enemies.enemy_exdeg_d;
