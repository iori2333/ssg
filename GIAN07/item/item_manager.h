///
/// ItemManager - Centralized item system state and operations
///

#pragma once

#include "core/object_pool.h"
#include "item.h"

class Player;

struct ItemManager {
  ObjectPool<ItemData, ITEM_MAX> pool;
  Player *player_ = nullptr;

  void Bind(Player &p) { player_ = &p; }

  void Init();
  void Spawn(int x, int y, uint8_t type);
  void Move();
  void Draw();
};


// === Backward-compatibility inline wrappers ===
