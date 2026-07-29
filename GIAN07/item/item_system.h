///
/// ItemSystem - collectible item entities and pickup processing
///

#pragma once

#include <cstdint>

#include "gfx/coords.h"
#include "util/object_pool.h"

class EffectManager;
class Player;

inline constexpr std::size_t kItemCapacity = 100;

enum class ItemKind : uint8_t {
  None,
  Score,
  Extend,
  Bomb,
};

inline constexpr auto kItemGravity = 3;
inline constexpr auto kItemHitRadius = 16_px;
inline constexpr auto kLargeItemHitRadius = 28_px;

struct ItemData {
  int x = 0;
  int y = 0;
  int vx = 0;
  int vy = 0;
  uint32_t count = 0;
  ItemKind kind = ItemKind::None;
  bool auto_collect = false;
};

class ItemSystem {
public:
  ItemSystem(Player &player, EffectManager &effects)
      : player_(player), effects_(effects) {}

  void Reset();
  void Spawn(int x, int y, ItemKind kind);
  void Update();
  void Draw() const;

private:
  [[nodiscard]] static int HitRadius(ItemKind kind);

  Player &player_;
  EffectManager &effects_;
  ObjectPool<ItemData, kItemCapacity> pool_;
};
