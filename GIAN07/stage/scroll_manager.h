/*
 *   ScrollManager — centralized scroll/scene system state and operations
 */

#pragma once

#include "scroll.h"
#include "game/coords.h"
#include <array>

struct ScrollManager {
  ScrollState scroll;     // ScrollInfo
  SceneState scene;       // SclInfo
  int key_wait_count = 0; // SclKeyWaitCount
  std::array<PIXEL_LTRB, 1200> map_chip_rects; // rcMapChip[]

  // === メソッド ===
  void Move();
  void Draw();
  void SetSpeed(int speed);
  void Command(uint8_t cmd);
  bool Init();
};

extern ScrollManager Scroller;
