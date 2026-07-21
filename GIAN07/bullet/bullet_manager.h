///
/// BulletManager - Centralized bullet and laser system state
///

#pragma once

#include <cstdint>

#include "bullet.h"
#include "core/object_pool.h"
#include "laser/homing.h"
#include "laser/long.h"
#include "laser/reflect.h"

struct EnemyData;
struct ItemManager;
struct GameManager;
class Player;

struct BulletManager {
  void Init();

  // --- DI ---
  void Bind(ItemManager &im) { items_ = &im; }
  void Bind(GameManager &gm) { game_ = &gm; }
  void Bind(Player &p) { player_ = &p; }

  // --- Bullet spawn ---
  bool Spawn(const BulletSpawnInfo &si);
  bool SpawnLine(const BulletSpawnInfo &si);
  bool SpawnExtra01(const BulletSpawnInfo &si);

  // --- Laser spawn ---
  bool SpawnReflect(const ReflectSpawnInfo &info);
  bool SpawnLongLaser(const LongLaserSpawnInfo &info);
  bool SpawnHoming(const HomingSpawnInfo &info);

  // --- Per-frame ---
  void UpdateAll();
  void HitCheckAll();
  void ClearAll();

  // --- Render (granular, matching game-loop draw order) ---
  void RenderAll() const;
  void RenderLong() const;
  void RenderHoming() const;
  void RenderReflect() const;

  // --- Laser control ---
  void ControlLongLaser(const EnemyData *e, uint8_t id,
                        const LongLaserUpdateInfo &info);
  void ClearHoming();

  // --- Bullet items / scoring ---
  uint32_t ScoreToItems();
  void ToItems(uint8_t n);

  // --- Debug ---
  void RenderDebugHitboxes(int mode) const;

  // --- Gallery ---
  void PlaceDisplayBullet(int x, int y, uint8_t color);
  void RotateDisplayAngles();

private:
  ItemManager *items_ = nullptr;
  GameManager *game_ = nullptr;
  Player *player_ = nullptr;

  // Bullet pools
  ObjectPool<Bullet, kBulletSmallMax> pool_small;
  ObjectPool<Bullet, kBulletLargeMax> pool_large;

  // Laser pools
  ObjectPool<LaserReflect, kReflectMax> reflect;
  ObjectPool<LaserLong, kLongLaserMax> long_lasers;
  ObjectPool<LaserHoming, kHomingMax> homing;

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

  void UpdateReflect();
  void UpdateLong();
  void UpdateHoming();
  void ClearReflect();
  void ClearLong();
};
