///
/// ScrollManager - Centralized scroll/scene system state and operations
///

#pragma once

#include <array>

#include "gfx/coords.h"
#include "scroll.h"

struct GameManager;

struct ScrollManager {
  ScrollState scroll;                          // ScrollInfo
  SceneState scene;                            // SclInfo
  int key_wait_count = 0;                      // SclKeyWaitCount
  std::array<PIXEL_LTRB, 1200> map_chip_rects; // rcMapChip[]

  GameManager *game_ = nullptr;

  // === Methods ===
  void Bind(GameManager &gm) { game_ = &gm; }
  void Move();
  void Draw();
  void SetSpeed(int speed);
  void Command(uint8_t cmd);
  bool Init();
};

extern ScrollManager Scroller;
