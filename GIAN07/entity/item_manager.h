/*
 *   ItemManager — centralized item system state
 */

#pragma once

#include "entity/ITEM.h"
#include "game/entity_pool.h"

struct ItemManager {
  EntityPool<ItemData, ITEM_MAX> pool;
};

extern ItemManager Items;
