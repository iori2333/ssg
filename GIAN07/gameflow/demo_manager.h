/*
 *   DemoManager — centralized demo/replay system state and operations
 */

#pragma once

#include "DEMOPLAY.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct DemoManager {
  bool load_enable = false;
  bool save_all_enable = false;
  bool load_all_enable = false;
  MULTI_REPLAY_INFO multi_play_info = {};
  uint8_t playback_max_stage = 0;
  std::u8string pending_replay_file;
  DemoPlayState demo_info = {};
  std::array<INPUT_BITS, DEMOBUF_MAX> demo_buffer = {};

  // ファイル静的変数（DEMOPLAY.cpp から移動）
  uint32_t demo_frame_cur = 0;
  struct ConfigTempData {
    uint8_t PlayerStock;
    uint8_t BombStock;
    uint8_t InputFlags;
  } config_temp;
  std::vector<std::vector<INPUT_BITS>> stage_record_bufs;
  uint8_t multi_stage_count = 0;
  uint8_t multi_stage_nums[REPLAY_STAGE_MAX] = {};
  uint32_t multi_stage_frames[REPLAY_STAGE_MAX] = {};
  std::vector<INPUT_BITS> all_playback_buf;

  // === メソッド ===
  void Init();
  [[nodiscard]] bool HasRecordedStages();
  void FlushStage();
  [[nodiscard]] bool LoadSetup();
  [[nodiscard]] bool Record(INPUT_BITS key);
  void SaveDemo();
  [[nodiscard]] bool LoadDemo(int stage);
  INPUT_BITS Move();
  void Cleanup();
  void SaveReplayAll(bool exstg);
  [[nodiscard]] bool LoadReplayAll(const char8_t *fn);
};

extern DemoManager Demos;
