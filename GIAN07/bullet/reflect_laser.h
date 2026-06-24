///
/// ReflectLaserSubsystem - short and reflective laser system.
///

#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "bullet_data.h"
#include "world_refs.h"

namespace bullets {

class LongLaserSubsystem; // forward

class ReflectLaserSubsystem {
public:
  ReflectLaserSubsystem(world::Refs w, LongLaserSubsystem &lon);

  void Reset();
  void Spawn(const LaserCommand &cmd); ///< Apply difficulty + rank.
  void SpawnEX(const LaserCommand &cmd);
  void Move();
  void Draw();
  void Clear();

  std::span<const LASER_DATA> Active() const;

private:
  world::Refs world_;
  LongLaserSubsystem &lon_;

  std::array<LASER_DATA, LASER_MAX> lasers_{};
  std::array<uint16_t, LASER_MAX> indices_{};
  uint16_t count_ = 0;

  void SetEasy(LaserCommand &cmd) const;
  void SetHard(LaserCommand &cmd) const;
  void SetLunatic(LaserCommand &cmd) const;
  uint8_t CalcDir(uint16_t i, const LaserCommand &cmd) const;
  static void SetupShort(LASER_DATA *lp);
  static void DrawShort(const LASER_DATA *lp);
  void MoveLaser(LASER_DATA *lp);
  static void HitCheck(LASER_DATA *lp, world::Refs w);
  void MoveReflect(LASER_DATA *lp);
  int HitReflect(const LASER_DATA *lp);
};

} // namespace bullets