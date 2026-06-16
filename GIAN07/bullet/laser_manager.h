/*
 *   LaserManager — centralized laser system state
 */

#pragma once

#include "HOMINGL.h"
#include "LASER.h"
#include "LLASER.h"
#include <array>
#include <cstdint>

struct LaserManager {
  // --- 反射レーザー ---
  LaserCommand cmd;              // LaserCmd
  uint16_t count = 0;            // LaserNow
  std::array<LASER_DATA, LASER_MAX> lasers;       // Laser[]
  std::array<uint16_t, LASER_MAX> laser_indices;  // LaserInd[]

  // --- 長レーザー ---
  std::array<LongLaserData, LLASER_MAX> long_lasers; // LLaser[]
  LongLaserCommand long_cmd;     // LLaserCmd

  // --- ホーミングレーザー ---
  uint16_t homing_count = 0;     // HLaserNow
  HomingLaserInfo homing_cmd;    // HLaserCmd
  std::array<HomingLaserData, HLASER_MAX> homing_buf; // HLaserBuf[]
  HomingLaserData active;        // ActiveHL
  HomingLaserData free_list;     // FreeHL

  // ================================================================
  // Reflective laser methods (was LASER.cpp free functions)
  // ================================================================
  void Spawn();                    // was laser_set (reads RankManager)
  void SpawnEX();                  // was laser_setEX
  int  SpawnLong(uint16_t* ind);   // was llaser_set
  void Move();                     // was laser_move
  void Draw();                     // was laser_draw
  void Clear();                    // was laser_clear
  void SetIndices();               // was laserind_set

private:
  void SetEasy();                  // was easy_cmdL
  void SetHard();                  // was hard_cmdL
  void SetLunatic();               // was luna_cmdL
  uint8_t CalcDir(uint16_t i);     // was laser_dir
  void SetupShort(LASER_DATA* lp); // was slaser_pset
  void DrawShort(const LASER_DATA* lp); // was SLdraw
  void MoveLaser(LASER_DATA* lp);  // was Lmove
  void HitCheck(LASER_DATA* lp);   // was laser_hitchk
  void MoveReflect(LASER_DATA* lp);// was REFL_move
  int  HitReflect(const LASER_DATA* lp); // was REFL_hit

public:
  // ================================================================
  // Long laser methods (was LLASER.cpp free functions)
  // ================================================================
  bool SpawnLongLaser(uint8_t id);                    // was LLaserSet
  void OpenLong(const EnemyData* e, uint8_t id);      // was LLaserOpen
  void CloseLong(const EnemyData* e, uint8_t id);     // was LLaserClose
  void LineLong(const EnemyData* e, uint8_t id);      // was LLaserLine
  void RotateLongAbs(const EnemyData* e, uint8_t d, uint8_t id);  // was LLaserDegA
  void RotateLongRel(const EnemyData* e, char d, uint8_t id);      // was LLaserDegR
  void ForceCloseLong(const EnemyData* e);             // was LLaserForceClose
  void MoveLong();             // was LLaserMove
  void DrawLong();             // was LLaserDraw
  void ClearLong();            // was LLaserClear
  void SetupLong();            // was LLaserSetup

private:
  void SetLongPoint(LLASER_DATA* lp);   // was _LLaserPointSet
  void HitCheckLong(const LLASER_DATA* lp); // was _LLaserHitCheck
  void UpdateLongXY(int id);            // was _LLaserXYSet

public:
  // ================================================================
  // Homing laser methods (was HOMINGL.cpp free functions)
  // ================================================================
  void InitHoming();           // was HLaserInit
  void SpawnHoming(const HomingLaserInfo* info); // was HLaserSet
  void MoveHoming();           // was HLaserMove (reads PlayerManager Viv)
  void DrawHoming();           // was HLaserDraw
  void ClearHoming();          // was HLaserClear
};

extern LaserManager Lasers;
