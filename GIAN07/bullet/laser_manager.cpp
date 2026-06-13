/*
 *   LaserManager — centralized laser system state
 */

#include "laser_manager.h"

// --- グローバルインスタンス ---
LaserManager Lasers;

// --- 後方互換用参照ラッパー ---
// クロスモジュール参照（enemy/, core/ で使用）
LaserCommand& LaserCmd = Lasers.cmd;
uint16_t& LaserNow = Lasers.count;
LongLaserCommand& LLaserCmd = Lasers.long_cmd;
uint16_t& HLaserNow = Lasers.homing_count;
// Laser, LaserInd, LLaser, HLaserCmd, HLaserBuf, ActiveHL, FreeHL → Lasers.xxx で直接アクセス
