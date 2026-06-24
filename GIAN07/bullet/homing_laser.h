///
/// HomingLaserSubsystem - homing / snake-tracking laser system.
///

#pragma once

#include <array>
#include <cstdint>

#include "bullet_data.h"
#include "world_refs.h"

namespace bullets {

class HomingLaserSubsystem {
public:
  explicit HomingLaserSubsystem(world::Refs w);

  void Init();
  void SpawnHoming(const HomingLaserInfo &info);
  void MoveHoming();
  void DrawHoming() const;
  void ClearHoming();

  const HomingLaserData *ActiveHead() const { return active_.Next; }

private:
  world::Refs world_;
  std::array<HomingLaserData, HLASER_MAX> buf_{};
  HomingLaserData active_{};
  HomingLaserData free_{};
  uint16_t count_ = 0;
};

} // namespace bullets