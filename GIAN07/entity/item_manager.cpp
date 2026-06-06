/*
 *   ItemManager — centralized item system state
 */

#include <array>
#include <cstdint>
#include "entity/item_manager.h"
#include "entity/ITEM.h"

ItemManager Items;
std::array<ItemData, ITEM_MAX> &Item = Items.pool.entities;
std::array<uint16_t, ITEM_MAX> &ItemInd = Items.pool.indices;
uint16_t &ItemNow = Items.pool.count;
