///
/// ProjectileSystem - aggregate subsystem dispatcher.
///

#include "projectile_system.h"

namespace bullets {

ProjectileSystem::ProjectileSystem(world::Refs w)
    : world_(w), bullets_(w), lon_(w), reflect_(w, lon_), homing_(w) {}

void ProjectileSystem::Reset() {
  bullets_.Reset();
  lon_.Setup();
  reflect_.Reset();
  homing_.Init();
}

void ProjectileSystem::Move() {
  bullets_.Move();
  reflect_.Move();
  lon_.MoveLong();
  homing_.MoveHoming();
}

// Draw order preserved from the original GameDraw in game_main.cpp:
//   - Long lasers are drawn in two passes gated by the active geometry
//     backend (GrpGeom_FB() vs GrpGeom_Poly()); each pass must run.
//   - Short/reflect lasers use geometry primitives, drawn once.
//   - Homing lasers also use geometry primitives.
//   - Enemy bullets draw last.
//
// The originals called Lasers.DrawLong() inside both `if (GrpGeom_FB())`
// and `if (GrpGeom_Poly())` branches: that intent is preserved here.
void ProjectileSystem::Draw() {
  lon_.DrawLong(); // internal pass performs its own backend gating
  homing_.DrawHoming();
  reflect_.Draw();
  bullets_.Draw();
}

void ProjectileSystem::ClearAll() {
  bullets_.Clear();
  reflect_.Clear();
  lon_.ClearLong();
  homing_.ClearHoming();
}

} // namespace bullets