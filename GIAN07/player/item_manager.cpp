/*
 *   ItemManager — centralized item system state
 */

#include "item_manager.h"

ItemManager Items;
std::array<ItemData, ITEM_MAX>& Item = Items.entities;
std::array<uint16_t, ITEM_MAX>& ItemInd = Items.indices;
uint16_t& ItemNow = Items.count;
