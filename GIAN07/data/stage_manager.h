///
/// StageManager — ECL/SCL/map script loading
///
#pragma once

#include <cstdint>

#include "core/game_manager.h"
#include "sys/buffer.h"

class StageManager {
public:
  bool LoadStageData(GameStage stage);
  BYTE_BUFFER_OWNED LoadDemo(int stage);
};

inline StageManager stage_mgr;
