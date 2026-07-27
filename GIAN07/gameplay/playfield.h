///
/// Playfield - shared gameplay area bounds
///

#pragma once

#include <cstdlib>

#include "gfx/constants.h"

namespace playfield {

inline constexpr WINDOW_COORD kLeft = 128;
inline constexpr WINDOW_COORD kRight = 511;
inline constexpr WINDOW_COORD kCenterX = (kLeft + kRight) / 2;
inline constexpr WINDOW_COORD kTop = 0;
inline constexpr WINDOW_COORD kBottom = GRP_RES.h - 1;
inline constexpr WINDOW_COORD kCenterY = (kTop + kBottom) / 2;

inline constexpr WINDOW_LTRB kClip = {kLeft, kTop, kRight + 1, kBottom + 1};

inline constexpr WORLD_COORD kWorldLeft = PixelToWorld(kLeft);
inline constexpr WORLD_COORD kWorldRight = PixelToWorld(kRight);
inline constexpr WORLD_COORD kWorldCenterX = (kWorldLeft + kWorldRight) / 2;
inline constexpr WORLD_COORD kWorldTop = PixelToWorld(kTop);
inline constexpr WORLD_COORD kWorldBottom = PixelToWorld(kBottom);
inline constexpr WORLD_COORD kWorldCenterY = (kWorldTop + kWorldBottom) / 2;

[[nodiscard]] inline bool WithinAxisDistance(WORLD_COORD lhs, WORLD_COORD rhs,
                                             WORLD_COORD distance) {
  return std::abs(lhs - rhs) < distance;
}

} // namespace playfield
