///
/// Playfield - shared gameplay area bounds
///

#pragma once

#include <cstdlib>

#include "gfx/core/constants.h"

namespace playfield {

inline constexpr int kLeft = 128;
inline constexpr int kRight = 511;
inline constexpr int kCenterX = (kLeft + kRight) / 2;
inline constexpr int kTop = 0;
inline constexpr int kBottom = kGameResolution.y - 1;
inline constexpr int kCenterY = (kTop + kBottom) / 2;

inline constexpr Rect kClip = {kLeft, kTop, kRight + 1, kBottom + 1};

inline constexpr WorldCoord kWorldLeft = PixelToWorld(kLeft);
inline constexpr WorldCoord kWorldRight = PixelToWorld(kRight);
inline constexpr WorldCoord kWorldCenterX = (kWorldLeft + kWorldRight) / 2;
inline constexpr WorldCoord kWorldTop = PixelToWorld(kTop);
inline constexpr WorldCoord kWorldBottom = PixelToWorld(kBottom);
inline constexpr WorldCoord kWorldCenterY = (kWorldTop + kWorldBottom) / 2;

[[nodiscard]] inline bool WithinAxisDistance(WorldCoord lhs, WorldCoord rhs,
                                             WorldCoord distance) {
  return (lhs - rhs).Abs() < distance;
}

} // namespace playfield
