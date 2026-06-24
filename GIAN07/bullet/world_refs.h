///
/// world_refs - Injected external world references for bullet/.
///
/// `bullets::world::get()` returns an aggregate of the existing game
/// singletons (Players / Enemies / Effects / Items / Ranking).  This is
/// the only header from `bullet/` that depends on external manager
/// headers, localizing the coupling that was previously spread across
/// bullet.cpp / laser.cpp / etc.
///

#pragma once

#include "effect/effect_manager.h"
#include "enemy/enemy_manager.h"
#include "gameflow/rank_manager.h"
#include "player/item_manager.h"
#include "player/player.h"

namespace bullets::world {

/// Aggregate of the external singletons projectiles need to touch.
struct Refs {
  Player &players;
  EnemyManager &enemies;
  EffectManager &effects;
  ItemManager &items;
  RankManager &ranking;
};

/// Returns a bundle referencing the game's global singletons.
Refs get();

} // namespace bullets::world