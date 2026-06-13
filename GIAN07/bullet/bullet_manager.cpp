/*
 *   BulletManager — centralized bullet system state
 */

#include "bullet_manager.h"

// --- グローバルインスタンス ---
BulletManager Bullets;

// --- 後方互換用参照ラッパー ---
// クロスモジュール参照（enemy/, player/ で使用）
std::array<Bullet, TAMA_MAX>& Tama = Bullets.bullets;
BulletCommand& TamaCmd = Bullets.command;
uint16_t& Tama1Now = Bullets.count_small;
uint16_t& Tama2Now = Bullets.count_large;
// Tama1Ind, Tama2Ind, Tama1Max, Tama2Max, TamaSpeed → Bullets.xxx で直接アクセス
