///
/// DemoManager - Centralized demo/replay system state and operations
///

#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "core/game_manager.h"
#include "data/game_data.h"
#include "demo_play.h"

struct DemoManager {
  explicit DemoManager(const data::GameData &data) : data_(&data) {}

  bool load_enable = false;
  bool save_all_enable = false;
  bool load_all_enable = false;
  MULTI_REPLAY_INFO multi_play_info = {};
  StageId playback_max_stage = StageId::STAGE_1;
  std::string pending_replay_file;
  DemoPlayState demo_info = {};
  std::array<INPUT_BITS, DEMOBUF_MAX> demo_buffer = {};

  // File-static variables (moved from DEMOPLAY.cpp)
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

  // === Methods ===
  void Init();
  [[nodiscard]] bool HasRecordedStages() const;
  void FlushStage();
  [[nodiscard]] bool LoadSetup();
  [[nodiscard]] bool Record(INPUT_BITS key);
  void UpdateLastRecordedInput(INPUT_BITS key);
  void SaveDemo();
  [[nodiscard]] bool LoadDemo(StageId stage);
  INPUT_BITS Move();
  void Cleanup();
  void SaveReplayAll(bool exstg);
  [[nodiscard]] bool LoadReplayAll(const char *fn);

private:
  const data::GameData *data_;
};

// Backward-compat: access via GameFlow.ctx.demos
