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

// === 後方互換 inline wrapper ===
inline void ScrollMove(void) { Scroller.Move(); }
inline void ScrollDraw(void) { Scroller.Draw(); }
inline void ScrollSpeed(int speed) { Scroller.SetSpeed(speed); }
inline void ScrollCommand(uint8_t cmd) { Scroller.Command(cmd); }
inline bool ScrollInit(void) { return Scroller.Init(); }
