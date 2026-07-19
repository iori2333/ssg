///
/// LaserManager - Centralized laser system state
///

#pragma once

#include <cstdint>

#include "core/laser_pool.h"
#include "laser/homing.h"
#include "laser/reflect.h"
#include "laser/long.h"

struct EnemyData;

struct LaserManager {
  LaserPool<LaserReflect, kReflectMax> reflect;
  LaserPool<LaserLong, kLongLaserMax> long_lasers;
  LaserPool<LaserHoming, kHomingMax> homing;

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

  template <typename Fn>
  void ApplyReflectLasers(Fn fn) {
    for (auto &r : reflect) {
      fn(r);
    }
  }

  template <typename Fn>
  void ApplyHomingLasers(Fn fn) {
    for (auto &h : homing) {
      fn(h);
    }
  }

  template <typename Fn>
  void ApplyLongLasers(const EnemyData *e, uint8_t id, Fn fn) {
    for (auto &ll : long_lasers) {
      if (ll.BelongsTo(e, id)) {
        fn(ll);
      }
    }
  }

  void RenderReflect() const;
  void RenderLong() const;
  void RenderHoming() const;
  void ClearReflect();
  void ClearLong();
  void ClearHoming();

private:
  void UpdateReflect();
  void UpdateLong();
  void UpdateHoming();
};

extern LaserManager Lasers;
