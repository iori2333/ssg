/*
 *   LaserManager — centralized laser system state
 */

#pragma once

#include "homing_laser.h"
#include "laser.h"
#include "long_laser.h"
#include <array>
#include <cstdint>

struct LaserManager {
  // --- 反射レーザー ---
  LaserCommand cmd{};                              // LaserCmd
  uint16_t count = 0;                              // LaserNow
  std::array<LASER_DATA, LASER_MAX> lasers{};      // Laser[]
  std::array<uint16_t, LASER_MAX> laser_indices{}; // LaserInd[]

  // --- 長レーザー ---
  std::array<LongLaserData, LLASER_MAX> long_lasers{}; // LLaser[]
  LongLaserCommand long_cmd{};                         // LLaserCmd

  // --- ホーミングレーザー ---
  uint16_t homing_count = 0;                            // HLaserNow
  HomingLaserInfo homing_cmd{};                         // HLaserCmd
  std::array<HomingLaserData, HLASER_MAX> homing_buf{}; // HLaserBuf[]
  HomingLaserData active{};                             // ActiveHL
  HomingLaserData free_list{};                          // FreeHL

  // ================================================================
  // Reflective laser methods (was LASER.cpp free functions)
  // ================================================================
  void Spawn();
  void SpawnEX();
  int SpawnLong(uint16_t *ind);
  void Move();
  void Draw();
  void Clear();
  void SetIndices();

private:
  void SetEasy();
  void SetHard();
  void SetLunatic();
  [[nodiscard]] uint8_t CalcDir(uint16_t i) const;
  static void SetupShort(LASER_DATA *lp);
  static void DrawShort(const LASER_DATA *lp);
  void MoveLaser(LASER_DATA *lp);
  static void HitCheck(LASER_DATA *lp);
  void MoveReflect(LASER_DATA *lp);
  int HitReflect(const LASER_DATA *lp);

public:
  // ================================================================
  // Long laser methods (was LLASER.cpp free functions)
  // ================================================================
  bool SpawnLongLaser(uint8_t id);
  void OpenLong(const EnemyData *e, uint8_t id);
  void CloseLong(const EnemyData *e, uint8_t id);
  void LineLong(const EnemyData *e, uint8_t id);
  void RotateLongAbs(const EnemyData *e, uint8_t d, uint8_t id);
  void RotateLongRel(const EnemyData *e, char d, uint8_t id);
  void ForceCloseLong(const EnemyData *e);
  void MoveLong();
  void DrawLong();
  void ClearLong();
  void SetupLong();

private:
  static void SetLongPoint(LongLaserData *lp);
  static void HitCheckLong(const LongLaserData *lp);
  void UpdateLongXY(int id);

public:
  // ================================================================
  // Homing laser methods (was HOMINGL.cpp free functions)
  // ================================================================
  void InitHoming();
  void SpawnHoming(const HomingLaserInfo *info);
  void MoveHoming();
  void DrawHoming() const;
  void ClearHoming();
};

extern LaserManager Lasers;
