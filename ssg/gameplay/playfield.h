///
/// Playfield - shared gameplay area bounds
///

#pragma once

#include <cstdlib>

#include "gfx/constants.h"

namespace playfield {

inline constexpr WindowCoord kLeft = 128;
inline constexpr WindowCoord kRight = 511;
inline constexpr WindowCoord kCenterX = (kLeft + kRight) / 2;
inline constexpr WindowCoord kTop = 0;
inline constexpr WindowCoord kBottom = kGameResolution.h - 1;
inline constexpr WindowCoord kCenterY = (kTop + kBottom) / 2;

inline constexpr WindowLtrb kClip = {kLeft, kTop, kRight + 1, kBottom + 1};

inline constexpr WorldCoord kWorldLeft = PixelToWorld(kLeft);
inline constexpr WorldCoord kWorldRight = PixelToWorld(kRight);
inline constexpr WorldCoord kWorldCenterX = (kWorldLeft + kWorldRight) / 2;
inline constexpr WorldCoord kWorldTop = PixelToWorld(kTop);
inline constexpr WorldCoord kWorldBottom = PixelToWorld(kBottom);
inline constexpr WorldCoord kWorldCenterY = (kWorldTop + kWorldBottom) / 2;

[[nodiscard]] inline bool WithinAxisDistance(WorldCoord lhs, WorldCoord rhs,
                                             WorldCoord distance) {
  return std::abs(lhs - rhs) < distance;
}

} // namespace playfield
