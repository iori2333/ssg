/*
 *   LaserManager — centralized laser system state
 */

#include <array>
#include <cstdint>

#include "entity/HOMINGL.h"
#include "entity/LASER.h"
#include "entity/LLASER.h"
#include "entity/laser_manager.h"

// --- グローバルインスタンス ---
LaserManager Lasers;

// --- 後方互換用参照ラッパー ---
LaserCommand &LaserCmd = Lasers.cmd;
uint16_t &LaserNow = Lasers.count;

std::array<LongLaserData, LLASER_MAX> &LLaser = Lasers.long_lasers;
LongLaserCommand &LLaserCmd = Lasers.long_cmd;

uint16_t &HLaserNow = Lasers.homing_count;
HomingLaserInfo &HLaserCmd = Lasers.homing_cmd;
std::array<HomingLaserData, HLASER_MAX> &HLaserBuf = Lasers.homing_buf;
HomingLaserData &ActiveHL = Lasers.active;
HomingLaserData &FreeHL = Lasers.free_list;
