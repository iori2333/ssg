/*
 *   ScrollManager — centralized scroll/scene system state
 */

#pragma once

#include "SCROLL.h"
#include "game/coords.h"
#include <array>

struct ScrollManager {
  ScrollState scroll;     // ScrollInfo
  SceneState scene;       // SclInfo
  int key_wait_count = 0; // SclKeyWaitCount
  std::array<PIXEL_LTRB, 1200> map_chip_rects; // rcMapChip[]
};

extern ScrollManager Scroller;
