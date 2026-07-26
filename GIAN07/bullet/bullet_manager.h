///
/// BulletManager - Centralized bullet and laser system state
///

#pragma once

#include <cstdint>

#include "bullet.h"
#include "laser/homing.h"
#include "laser/long.h"
#include "laser/reflect.h"

#include "core/object_pool.h"

struct EnemyActor;
struct EnemyHomingTarget;
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
  bool SpawnBullet(const BulletSpawnInfo &si);

  // --- Laser spawn ---
  bool SpawnReflect(const ReflectSpawnInfo &info);
  bool SpawnLongLaser(const LongLaserSpawnInfo &info);
  bool SpawnHoming(const HomingSpawnInfo &info);

  // --- Per-frame ---
  void Update(const EnemyHomingTarget &target);
  void HitCheck();
  void Clear();

  // --- Render ---
  void Render() const;

  // --- Laser control ---
  void ControlLongLaser(const EnemyActor *e, uint8_t id,
                        const LongLaserUpdateInfo &info);

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

  // Bullet pool
  ObjectPool<Bullet, kBulletMax> pool;

  // Laser pools
  ObjectPool<LaserReflect, kReflectMax> reflect;
  ObjectPool<LaserLong, kLongLaserMax> long_lasers;
  ObjectPool<LaserHoming, kHomingMax> homing;

  void UpdateBullet(const EnemyHomingTarget &target);
  void UpdateReflect();

  bool SpawnBulletNormal(const BulletSpawnInfo &si);
  bool SpawnBulletLine(const BulletSpawnInfo &si);
  bool SpawnBulletExtra01(const BulletSpawnInfo &si);
  void UpdateLong();
  void UpdateHoming();
};
