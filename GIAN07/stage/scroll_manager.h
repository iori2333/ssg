/*
 *   ScrollManager — centralized scroll/scene system state and operations
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

  // === メソッド ===
  void Move();                   // was ScrollMove
  void Draw();                   // was ScrollDraw
  void SetSpeed(int speed);      // was ScrollSpeed
  void Command(uint8_t cmd);     // was ScrollCommand
  bool Init();                   // was ScrollInit
};

extern ScrollManager Scroller;
