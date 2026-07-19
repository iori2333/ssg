///
/// LaserBase - Abstract base class for all laser types
///

#pragma once

#include <cstdint>

// Common fields shared by all three laser types:
//   - LaserReflect (short / reflective laser)
//   - LaserLong (thick infinite beam anchored to an enemy)
//   - LaserHoming (homing snake laser)
//
// Each derived type implements the pure-virtual lifecycle:
//   Move()      — per-frame physics / state update
//   Draw()      — render geometry
//   HitCheck()  — collision detection against the player
//   IsDead()    — whether this laser should be removed from the active pool
//   StartClear()— begin the death / clear animation

struct LaserBase {
  int x, y;         // Reference position (x64 fixed-point)
  int v;             // Velocity

  uint8_t d;         // Direction angle (0–255)
  uint8_t c;         // Color index
  uint8_t type;      // Sub-type discriminator
  uint8_t flag;      // State / life-cycle flag

  uint32_t count;    // Frame counter

  virtual ~LaserBase() = default;

  virtual void Move() = 0;
  virtual void Draw() const = 0;
  virtual void HitCheck() = 0;
  virtual bool IsDead() const = 0;
  virtual void StartClear() = 0;
};
