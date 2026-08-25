///
/// Shared stage command enums consumed by both the stage system and the ECL
/// decoder. Kept free of stage implementation headers so the pure ECL decoder
/// does not drag in the stage subsystem.
///

#pragma once

#include <cstdint>

namespace stage {

enum class Stage4RockCommand : uint8_t {
  Normal = 0,
  Accelerate = 1,
  Reverse = 2,
  Rotate = 3,
  Leave = 4,
  End = 5,
};

} // namespace stage