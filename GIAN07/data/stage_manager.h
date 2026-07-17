///
/// StageManager — ECL/SCL/map script loading
///
#pragma once

#include <cstdint>

#include "sys/buffer.h"

class StageManager {
public:
  bool LoadStageData(uint8_t stage);
  BYTE_BUFFER_OWNED LoadDemo(int stage);
};

inline StageManager stage_mgr;
