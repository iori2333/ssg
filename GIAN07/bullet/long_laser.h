///
/// LongLaserSubsystem - thick / infinite laser system.
///

#pragma once

#include <array>
#include <cstdint>
#include <span>

#include "bullet_data.h"
#include "world_refs.h"

namespace bullets {

class LongLaserSubsystem {
public:
  explicit LongLaserSubsystem(world::Refs w);

  void Setup();
  bool SpawnLongLaser(const LongLaserCommand &cmd, uint8_t id);
  void OpenLong(const EnemyData *e, uint8_t id);
  void CloseLong(const EnemyData *e, uint8_t id);
  void LineLong(const EnemyData *e, uint8_t id);
  void RotateLongAbs(const EnemyData *e, uint8_t d, uint8_t id);
  void RotateLongRel(const EnemyData *e, char d, uint8_t id);
  void ForceCloseLong(const EnemyData *e);

  void MoveLong();
  void DrawLong();
  void ClearLong();

  std::span<LongLaserData> AllUnsafe() { return long_lasers_; }
  std::span<const LongLaserData> All() const { return long_lasers_; }

private:
  world::Refs world_;
  std::array<LongLaserData, LLASER_MAX> long_lasers_{};

  static void SetLongPoint(LongLaserData *lp);
  static void HitCheckLong(const LongLaserData *lp, world::Refs w);
  void UpdateLongXY(int id);
};

} // namespace bullets