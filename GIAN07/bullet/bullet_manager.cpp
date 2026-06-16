/*
 *   BulletManager — centralized bullet system state
 */

#include "bullet_manager.h"

// --- グローバルインスタンス ---
BulletManager Bullets;

// TamaCmd, Tama は TAMA.h inline 関数（TamaSTDForm 等）が参照するため保持
BulletCommand& TamaCmd = Bullets.command;
std::array<Bullet, TAMA_MAX>& Tama = Bullets.bullets;
