///
/// ItemManager - Centralized item system state and operations
///

#pragma once

#include "item.h"

struct ItemManager {
  std::array<ItemData, ITEM_MAX> entities; // Items.entities[]
  std::array<uint16_t, ITEM_MAX> indices;  // Items.indices[]
  uint16_t count = 0;                      // Items.count

  // === Methods ===
  void Spawn(int x, int y, uint8_t type);
  void Move();
  void Draw();
  void SetIndices();
};


// === Backward-compatibility inline wrappers ===
