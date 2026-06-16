/*
 *   ItemManager — centralized item system state and operations
 */

#pragma once

#include "ITEM.h"

struct ItemManager {
  std::array<ItemData, ITEM_MAX> entities;  // Item[]
  std::array<uint16_t, ITEM_MAX> indices;    // ItemInd[]
  uint16_t count = 0;                         // ItemNow

  // === メソッド ===
  void Spawn(int x, int y, uint8_t type);   // was ItemSet
  void Move();                               // was ItemMove
  void Draw();                               // was ItemDraw
  void SetIndices();                         // was ItemIndSet
};

extern ItemManager Items;

// === 後方互換 inline wrapper ===
inline void ItemSet(int x, int y, uint8_t type) { Items.Spawn(x, y, type); }
inline void ItemMove(void) { Items.Move(); }
inline void ItemDraw(void) { Items.Draw(); }
inline void ItemIndSet(void) { Items.SetIndices(); }
