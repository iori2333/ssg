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
