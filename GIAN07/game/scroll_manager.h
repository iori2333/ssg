/*
 *   ScrollManager — centralized scroll/scene system state
 */

#pragma once

#include "game/SCROLL.h"

struct ScrollManager {
  ScrollState scroll;     // ScrollInfo
  SceneState scene;       // SclInfo
  int key_wait_count = 0; // SclKeyWaitCount
};

extern ScrollManager Scroller;
