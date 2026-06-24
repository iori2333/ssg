///
/// ProjectileSystem - Modern projectile (bullet + laser) subsystem.
///
/// Replaces the legacy scattered globals `Bullets` / `Lasers` with a
/// single aggregated system injected with external world references.
///

#pragma once

#include "bullet_subsystem.h"
#include "homing_laser.h"
#include "long_laser.h"
#include "reflect_laser.h"
#include "world_refs.h"

namespace bullets {

class ProjectileSystem {
public:
  explicit ProjectileSystem(world::Refs w);

  void Reset(); ///< Initialize all sub-pools (replaces SetIndices / SetupLong /
                ///< InitHoming).
  void Move();  ///< Per-frame update of enemy bullets & all lasers.
  void Draw(); ///< Per-frame draw, preserving the original multi-pass ordering.
  void
  ClearAll(); ///< Clear every active projectile (player death / stage switch).

  BulletSubsystem &Bullets() { return bullets_; }
  ReflectLaserSubsystem &Reflect() { return reflect_; }
  LongLaserSubsystem &Long() { return lon_; }
  HomingLaserSubsystem &Homing() { return homing_; }

  const BulletSubsystem &Bullets() const { return bullets_; }
  const ReflectLaserSubsystem &Reflect() const { return reflect_; }
  const LongLaserSubsystem &Long() const { return lon_; }
  const HomingLaserSubsystem &Homing() const { return homing_; }

  world::Refs &World() { return world_; }

private:
  world::Refs world_;
  BulletSubsystem bullets_;
  ReflectLaserSubsystem reflect_;
  LongLaserSubsystem lon_;
  HomingLaserSubsystem homing_;
};

} // namespace bullets