///
/// World-coordinate math adapters
///
#pragma once

#include <cmath>
#include <cstdint>

#include "coords.h"

#include "util/math_utils.h"

namespace math {

[[nodiscard]] inline WorldPoint RoundedPolarVector(float angle,
                                                   WorldCoord length) {
  const auto vector = RoundedPolarVector(angle, length.Raw());
  return {WorldCoord::FromRaw(vector.x), WorldCoord::FromRaw(vector.y)};
}

[[nodiscard]] inline WorldPoint RoundedRotateVector(float angle,
                                                    WorldPoint vector) {
  const auto rotated =
      RoundedRotateVector(angle, vector.x.Raw(), vector.y.Raw());
  return {WorldCoord::FromRaw(rotated.x), WorldCoord::FromRaw(rotated.y)};
}

// Rotates a 3D point (any type exposing WorldCoord .x/.y/.z) around X, Y, and
// Z in the legacy 256-angle system. Shared by the stage visuals and the effect
// manager so the rotation order cannot drift.
template <typename Point>
inline void RotatePoint3D(Point &point, uint8_t angle_x, uint8_t angle_y,
                          uint8_t angle_z) {
  auto rotated = RoundedRotateVector(AngleFromLegacy(angle_x),
                                     WorldPoint{point.y, point.z});
  point.y = rotated.x;
  point.z = rotated.y;

  rotated = RoundedRotateVector(-AngleFromLegacy(angle_y),
                                WorldPoint{point.x, point.z});
  point.x = rotated.x;
  point.z = rotated.y;

  rotated = RoundedRotateVector(AngleFromLegacy(angle_z),
                                WorldPoint{point.x, point.y});
  point.x = rotated.x;
  point.y = rotated.y;
}

[[nodiscard]] inline float AngleTo(WorldPoint vector) {
  return AngleTo(static_cast<float>(vector.x.Raw()),
                 static_cast<float>(vector.y.Raw()));
}

[[nodiscard]] inline WorldCoord RoundedLength(WorldPoint vector) {
  const auto x = static_cast<int64_t>(vector.x.Raw());
  const auto y = static_cast<int64_t>(vector.y.Raw());
  const auto length = std::sqrt(static_cast<double>(x * x + y * y));
  return WorldCoord::FromRaw(static_cast<int>(std::lround(length)));
}

[[nodiscard]] inline bool WithinRadius(WorldPoint lhs, WorldPoint rhs,
                                       WorldCoord radius) {
  const auto dx = static_cast<int64_t>((lhs.x - rhs.x).Raw());
  const auto dy = static_cast<int64_t>((lhs.y - rhs.y).Raw());
  const auto r = static_cast<int64_t>(radius.Raw());
  return dx * dx + dy * dy < r * r;
}

[[nodiscard]] inline bool WithinRadiusOrBoundary(WorldPoint lhs, WorldPoint rhs,
                                                 WorldCoord radius) {
  const auto dx = static_cast<int64_t>((lhs.x - rhs.x).Raw());
  const auto dy = static_cast<int64_t>((lhs.y - rhs.y).Raw());
  const auto r = static_cast<int64_t>(radius.Raw());
  return dx * dx + dy * dy <= r * r;
}

[[nodiscard]] inline WorldCoord RandomWorldBelow(WorldCoord upper_bound) {
  return WorldCoord::FromRaw(RandomInt() % upper_bound.Raw());
}

} // namespace math
