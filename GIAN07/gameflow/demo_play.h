///
/// DemoPlay - Demo playback
///

#pragma once

#include <string>
#include <vector>

#include "sys/input.h"

struct ConfigData;

// [Constants]
inline constexpr auto DEMOBUF_MAX =
    (60 * 60 * 30); // Can store 30 minutes of data
static constexpr auto REPLAY_STAGE_MAX = 6;

// Replay-specific config option subset
// The original code simply reused ConfigData, which we can't do in this fork
// due to the additional fields we add to the structure.
struct DEMOPLAY_ConfigData {
  uint8_t GameLevel;
  uint8_t PlayerStock;
  uint8_t BombStock;
  uint8_t Padding1[5] = {0};
  uint8_t InputFlags;
  uint8_t Padding2[15] = {0};
};
static_assert(sizeof(DEMOPLAY_ConfigData) == 24);

// [Structs]
struct DemoPlayState {
  uint32_t RndSeed;           // Random seed
  uint32_t FrameCount;        // Not data size! Including the terminating ESC.
  DEMOPLAY_ConfigData CfgDat; // Config data (partially referenced on Load)
  uint8_t Exp;                // Initial power-up
  uint8_t Weapon;             // Initial weapon
};
// (DEMOPLAY_INFO alias removed — use DemoPlayState directly)

// Multi-stage replay header
struct MULTI_REPLAY_INFO {
  uint32_t RndSeed;
  uint8_t StageCount;         // Number of stages recorded (1-6)
  DEMOPLAY_ConfigData CfgDat; // 24 bytes
  uint8_t Exp;
  uint8_t Weapon;
  uint8_t Stages[REPLAY_STAGE_MAX];       // Stage numbers
  uint32_t FrameCounts[REPLAY_STAGE_MAX]; // Per-stage input frame count
};

// [Functions]
// Backward-compat inline wrappers moved to end of demo_manager.h
// Implementation migrated to DemoManager methods

// [Variables]
// File-static variables moved to DemoManager struct in demo_manager.h.
// Access via Demos.load_enable, Demos.save_all_enable, Demos.load_all_enable,
// Demos.playback_max_stage, Demos.pending_replay_file
