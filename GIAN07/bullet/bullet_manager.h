///
/// BulletManager - Centralized bullet system state
///

#pragma once

#include <cstdint>

#include "bullet.h"
#include "core/object_pool.h"

struct BulletManager {
  void Init();

  bool Spawn(const BulletSpawnInfo &si);
  bool SpawnLine(const BulletSpawnInfo &si);
  bool SpawnExtra01(const BulletSpawnInfo &si);

  void UpdateAll();
  void HitCheckAll();
  void RenderAll() const;
  void ClearAll();

  void Move() { UpdateAll(); }
  void Draw() { RenderAll(); }
  void Clear() { ClearAll(); }

  uint32_t ScoreToItems();
  void ToItems(uint8_t n);
  void RenderDebugHitboxes(int mode) const;

  void PlaceDisplayBullet(int x, int y, uint8_t color);
  void RotateDisplayAngles();

private:
  ObjectPool<Bullet, kBulletSmallMax> pool_small;
  ObjectPool<Bullet, kBulletLargeMax> pool_large;

  template <typename Pool>
  void SpawnImpl(const BulletSpawnInfo &si, Pool &pool);

  template <typename Fn> void ApplySmall(Fn fn) {
    for (auto &b : pool_small) {
      fn(b);
    }
  }
  template <typename Fn> void ApplyLarge(Fn fn) {
    for (auto &b : pool_large) {
      fn(b);
    }
  }
};

extern BulletManager Bullets;
