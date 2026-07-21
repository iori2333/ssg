///
/// ScrollManager - Centralized scroll/scene system state and operations
///

#pragma once

#include <array>

#include "core/config.h"
#include "gfx/coords.h"
#include "scroll.h"

struct GameManager;
class Player;

struct ScrollManager {
  ScrollState scroll;                          // ScrollInfo
  SceneState scene;                            // SclInfo
  int key_wait_count = 0;                      // SclKeyWaitCount
  std::array<PIXEL_LTRB, 1200> map_chip_rects; // rcMapChip[]

  GameManager *game_ = nullptr;
  Player *player_ = nullptr;
  const GraphicsConfig *graphics_cfg_ = nullptr;

  // === Methods ===
  void Bind(GameManager &gm) { game_ = &gm; }
  void Bind(Player &p) { player_ = &p; }
  void Bind(const GraphicsConfig &gc) { graphics_cfg_ = &gc; }
  void Move();
  void Draw();
  void SetSpeed(int speed);
  void Command(uint8_t cmd);
  bool Init();
};

extern ScrollManager Scroller;
