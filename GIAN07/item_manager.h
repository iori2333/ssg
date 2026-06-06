/*
 *   ItemManager — centralized item system state
 */

#pragma once

#include "ITEM.h"

struct ItemManager {
  std::array<ItemData, ITEM_MAX> entities;  // Item[]
  std::array<uint16_t, ITEM_MAX> indices;    // ItemInd[]
  uint16_t count = 0;                         // ItemNow
};

extern ItemManager Items;
