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
  ObjectPool<LaserReflect, kReflectMax> reflect;
  ObjectPool<LaserLong, kLongLaserMax> long_lasers;
  ObjectPool<LaserHoming, kHomingMax> homing;

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

  void RenderReflect() const;
  void RenderLong() const;
  void RenderHoming() const;
  void ClearReflect();
  void ClearLong();
  void ClearHoming();

  void RenderDebugHitboxes(int mode) const;

private:
  void UpdateReflect();
  void UpdateLong();
  void UpdateHoming();
};

extern LaserManager Lasers;
