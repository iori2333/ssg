///
/// ItemManager - Centralized item system state and operations
///

#pragma once

#include "item.h"

#include "util/object_pool.h"

class Player;
class EffectManager;

class ItemManager {
public:
  ItemManager(Player &player, EffectManager &effects)
      : player_(player), effects_(effects) {}

  void Init();
  void Spawn(int x, int y, uint8_t type);
  void Move();
  void Draw() const;

private:
  Player &player_;
  EffectManager &effects_;
  ObjectPool<ItemData, ITEM_MAX> pool_;
};
