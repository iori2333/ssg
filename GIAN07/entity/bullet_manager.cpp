/*
 *   BulletManager — centralized bullet system state
 */

#include "entity/bullet_manager.h"
#include <array>
#include "entity/TAMA.h"
#include <cstdint>

// --- グローバルインスタンス ---
BulletManager Bullets;

// --- 後方互換用参照ラッパー ---
std::array<Bullet, TAMA_MAX> &Tama = Bullets.bullets;
BulletCommand &TamaCmd = Bullets.command;
std::array<uint16_t, TAMA_MAX> &Tama1Ind = Bullets.indices_small;
std::array<uint16_t, TAMA_MAX> &Tama2Ind = Bullets.indices_large;
uint16_t &Tama1Now = Bullets.count_small;
uint16_t &Tama2Now = Bullets.count_large;
uint16_t &Tama1Max = Bullets.max_small;
uint16_t &Tama2Max = Bullets.max_large;
int &TamaSpeed = Bullets.speed;
