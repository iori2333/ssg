///
/// world - Aggregated game-world singletons including the modern
/// ProjectileSystem.  Exposes `gWorld` as the single symbol that
/// non-bullet code touches instead of the deleted `Bullets` / `Lasers`.
///

#pragma once

#include "bullet/projectile_system.h"

struct GameWorld {
  bullets::ProjectileSystem projectiles{bullets::world::get()};
};

/// Returns the game world.  Lazily initialized on first access to
/// ensure the existing global singletons (Players, Enemies, ...) are
/// already constructed when `ProjectileSystem` captures references to
/// them.
GameWorld &gWorld();