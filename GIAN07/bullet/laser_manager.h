///
/// LaserManager - Centralized laser system state
///

#pragma once

#include <array>
#include <cstdint>

#include "core/laser_pool.h"
#include "laser_reflect.h"
#include "long_laser.h"
#include "homing_laser.h"

struct LaserManager {
  // --- Pools ---
  LaserPool<LaserReflect, LASER_MAX> reflect;
  LaserPool<LaserLong, LLASER_MAX> long_lasers;
  LaserPool<LaserHoming, HLASER_MAX> homing;

  // --- Command buffers ---
  LaserCommand cmd{};
  LongLaserCommand long_cmd{};
  HomingLaserInfo homing_cmd{};

  // ================================================================
  //  Init
  // ================================================================
  void SetIndices();
  void SetupLong();
  void InitHoming();

  // ================================================================
  //  Spawn (signatures preserved for ECL compatibility)
  // ================================================================
  void Spawn();
  void SpawnEX();
  bool SpawnLongLaser(uint8_t id);
  void SpawnHoming(const HomingLaserInfo *info);

  // ================================================================
  //  Per-frame (MoveAll / DrawAll / ClearAll are the unified entry
  //  points; old names kept as inline wrappers for callers)
  // ================================================================
  void MoveAll();
  void DrawAll() const;
  void ClearAll();

  // backward-compat wrappers
  void Move()   { MoveAll(); }
  void Draw()   { DrawAll(); }
  void Clear()  { ClearAll(); }

  // ================================================================
  //  Long laser controls (unchanged signatures)
  // ================================================================
  void OpenLong(const EnemyData *e, uint8_t id);
  void CloseLong(const EnemyData *e, uint8_t id);
  void LineLong(const EnemyData *e, uint8_t id);
  void RotateLongAbs(const EnemyData *e, uint8_t d, uint8_t id);
  void RotateLongRel(const EnemyData *e, char d, uint8_t id);
  void ForceCloseLong(const EnemyData *e);
  void MoveLong();
  void DrawLong() const;
  void ClearLong();

  // ================================================================
  //  Homing laser (unchanged signatures — will convert in Phase 4)
  // ================================================================
  void MoveHoming();
  void DrawHoming() const;
  void ClearHoming();

  // ================================================================
  //  Cross-pool helpers
  // ================================================================
  int HitReflect(const LaserReflect *lp);

  // --- Long laser geometry helper (used by LaserLong::Move) ---
  void SetLongPoint(LaserLong *lp);

private:
  // --- Difficulty scaling ---
  void SetEasy();
  void SetHard();
  void SetLunatic();
  [[nodiscard]] uint8_t CalcDir(uint16_t i) const;
};

extern LaserManager Lasers;
