///
/// ItemSystem - collectible item entities and pickup processing
///

#pragma once

#include <cstdint>

#include "gfx/coords.h"
#include "util/object_pool.h"

class EffectManager;
class Player;

inline constexpr auto ITEM_MAX = 100;

inline constexpr auto ITEM_DELETE = 0x00;
inline constexpr auto ITEM_SCORE = 0x01;
inline constexpr auto ITEM_EXTEND = 0x02;
inline constexpr auto ITEM_BOMB = 0x03;

inline constexpr auto ITEM_GRAVITY = 3;
inline constexpr auto ITEM_HIT_RADIUS = 16_px;
inline constexpr auto ITEM_HIT_RADIUS_LARGE = 28_px;

struct ItemData {
  int x = 0;
  int y = 0;
  int vx = 0;
  int vy = 0;
  uint32_t count = 0;
  uint8_t type = ITEM_DELETE;
  bool auto_collect = false;
};

class ItemSystem {
public:
  ItemSystem(Player &player, EffectManager &effects)
      : player_(player), effects_(effects) {}

  void Reset();
  void Spawn(int x, int y, uint8_t type);
  void Update();
  void Draw() const;

private:
  [[nodiscard]] static int HitRadius(uint8_t type);

  Player &player_;
  EffectManager &effects_;
  ObjectPool<ItemData, ITEM_MAX> pool_;
};
