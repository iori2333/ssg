/*
 *   ItemManager — centralized item system state and operations
 */

#pragma once

#include "ITEM.h"

struct ItemManager {
  std::array<ItemData, ITEM_MAX> entities;  // Items.entities[]
  std::array<uint16_t, ITEM_MAX> indices;    // Items.indices[]
  uint16_t count = 0;                         // Items.count

  // === メソッド ===
  void Spawn(int x, int y, uint8_t type);   // was ItemSet
  void Move();                               // was ItemMove
  void Draw();                               // was ItemDraw
  void SetIndices();                         // was ItemIndSet
};

extern ItemManager Items;

// === 後方互換 inline wrapper ===
