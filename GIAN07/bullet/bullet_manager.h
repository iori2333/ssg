///
/// BulletManager - Centralized bullet and laser system state
///

#pragma once

#include <cstdint>

#include "bullet.h"
#include "laser/homing.h"
#include "laser/long.h"
#include "laser/reflect.h"

#include "util/object_pool.h"

struct EnemyActor;
struct EnemyHomingTarget;
class EffectManager;
class ItemSystem;
struct GameSession;
class Player;

class BulletManager {
public:
  BulletManager(ItemSystem &items, GameSession &session, Player &player,
                EffectManager &effects)
      : items_(items), session_(session), player_(player), effects_(effects) {}

  void Init();

  // --- Bullet spawn ---
  void SpawnBullet(const BulletSpawnInfo &si);

  // --- Laser spawn ---
  void SpawnReflect(const ReflectSpawnInfo &info);
  bool SpawnLongLaser(const LongLaserSpawnInfo &info);
  void SpawnHoming(const HomingSpawnInfo &info);

  // --- Per-frame ---
  void Update(const EnemyHomingTarget &target);
  void Clear();

  // --- Render ---
  void Render() const;

  // --- Laser control ---
  void ControlLongLaser(const EnemyActor *e, uint8_t id,
                        const LongLaserUpdateInfo &info);

  // --- Bullet items / scoring ---
  uint32_t ConvertBulletsToScore();
  void ConvertBulletsToItems(uint8_t frequency);

  // --- Debug ---
  void RenderDebugHitboxes(int mode) const;

  // --- Gallery ---
  void PlaceDisplayBullet(int x, int y, uint8_t color);
  void RotateDisplayAngles();

private:
  ItemSystem &items_;
  GameSession &session_;
  Player &player_;
  EffectManager &effects_;

  ObjectPool<Bullet, kBulletMax> bullets_;
  ObjectPool<LaserReflect, kReflectMax> reflect_lasers_;
  ObjectPool<LaserLong, kLongLaserMax> long_lasers_;
  ObjectPool<LaserHoming, kHomingMax> homing_lasers_;

  void UpdateBullet(const EnemyHomingTarget &target);
  void UpdateReflect();

  void SpawnBulletNormal(const BulletSpawnInfo &si);
  void SpawnBulletLine(const BulletSpawnInfo &si);
  void SpawnBulletExtra01(const BulletSpawnInfo &si);
  void UpdateLong();
  void UpdateHoming();
  void HitCheck();
};
