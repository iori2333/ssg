/*
 *   DemoManager — centralized demo/replay system state
 */

#pragma once

#include "DEMOPLAY.h"
#include <array>
#include <cstdint>
#include <string>

struct DemoManager {
  bool load_enable = false;
  bool save_all_enable = false;
  bool load_all_enable = false;
  MULTI_REPLAY_INFO multi_play_info = {};
  uint8_t playback_max_stage = 0;
  std::u8string pending_replay_file;
  DEMOPLAY_INFO demo_info = {};
  std::array<INPUT_BITS, DEMOBUF_MAX> demo_buffer = {};
};

extern DemoManager Demos;
// 後方互換用参照は DEMOPLAY.h で宣言
