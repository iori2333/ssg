///
/// LaserManager - Centralized laser system state
///

#pragma once

#include <cstdint>

#include "core/object_pool.h"
#include "laser/homing.h"
#include "laser/reflect.h"
#include "laser/long.h"

struct EnemyData;

struct LaserManager {
  void Init();

  bool SpawnReflect(const ReflectSpawnInfo &info);
  bool SpawnLongLaser(const LongLaserSpawnInfo &info);
  bool SpawnHoming(const HomingSpawnInfo &info);

  void UpdateAll();
  void HitCheckAll() const;
  void RenderAll() const;
  void ClearAll();

  void Move() { UpdateAll(); }
  void Draw() { RenderAll(); }
  void Clear() { ClearAll(); }

  void ControlLongLaser(const EnemyData *e, uint8_t id,
                        const LongLaserUpdateInfo &info);

  void RenderLong() const;
  void RenderHoming() const;
  void ClearHoming();

  void RenderDebugHitboxes(int mode) const;

private:
  ObjectPool<LaserReflect, kReflectMax> reflect;
  ObjectPool<LaserLong, kLongLaserMax> long_lasers;
  ObjectPool<LaserHoming, kHomingMax> homing;

  void UpdateReflect();
  void UpdateLong();
  void UpdateHoming();
  void RenderReflect() const;
  void ClearReflect();
  void ClearLong();
};

extern LaserManager Lasers;
