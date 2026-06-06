/*
 *   ItemManager — centralized item system state
 */

#include "entity/item_manager.h"
#include "entity/ITEM.h"
#include <array>
#include <cstdint>

ItemManager Items;
std::array<ItemData, ITEM_MAX> &Item = Items.pool.entities;
std::array<uint16_t, ITEM_MAX> &ItemInd = Items.pool.indices;
uint16_t &ItemNow = Items.pool.count;
