///
/// world_refs - implementation of world::get() — the single place that
/// binds `bullets::` to the game's existing globals.
///

#include "world_refs.h"

namespace bullets::world {

Refs get() {
  return Refs{
      Players, Enemies, Effects, Items, Ranking,
  };
}

} // namespace bullets::world