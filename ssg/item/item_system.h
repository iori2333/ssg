///
/// ItemSystem - collectible item entities and pickup processing
///

#pragma once

#include <cstdint>

#include "gfx/core/coords.h"
#include "util/object_pool.h"

class EffectManager;
class Player;

namespace audio {
class AudioSystem;
}

inline constexpr std::size_t kItemCapacity = 100;

enum class ItemKind : uint8_t {
  None,
  Score,
  Extend,
  Bomb,
};

inline constexpr auto kItemGravity = WorldCoord::FromRaw(3);
inline constexpr auto kItemHitRadius = 16_px;
inline constexpr auto kLargeItemHitRadius = 28_px;

struct ItemData {
  WorldCoord x{};
  WorldCoord y{};
  WorldCoord vx{};
  WorldCoord vy{};
  int count = 0;
  ItemKind kind = ItemKind::None;
  bool auto_collect = false;
};

class ItemSystem {
public:
  ItemSystem(Player &player, EffectManager &effects, audio::AudioSystem &audio)
      : player_(player), effects_(effects), audio_(audio) {}

  void Reset();
  void Spawn(WorldCoord x, WorldCoord y, ItemKind kind);
  void Update();
  void Draw() const;

private:
  [[nodiscard]] static WorldCoord HitRadius(ItemKind kind);

  Player &player_;
  EffectManager &effects_;
  audio::AudioSystem &audio_;
  util::ObjectPool<ItemData, kItemCapacity> pool_;
};
