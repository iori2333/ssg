///
/// Geometry - Backend-independent geometry rendering functions
///

#include "geometry.h"

#include "util/math_utils.h"

namespace geometry {

constexpr uint8_t kCircleStep = (0x100 / (kCirclePointCount - 1));

void ApproximateCircle(std::span<VertexXy, kCirclePointCount> ret,
                       WindowPoint center, PixelCoord radius) {
  auto i = 0;
  for (auto &v : ret) {
    const uint8_t angle = (i++ * kCircleStep);
    const auto offset =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), radius);
    v.x = static_cast<VertexCoord>(center.x + offset.x);
    v.y = static_cast<VertexCoord>(center.y + offset.y);
  }
}

void ApproximateFatCircle(std::span<VertexXy, (kCirclePointCount * 2)> ret,
                          WindowPoint center, PixelCoord r, PixelCoord w) {
  auto v = ret.begin();
  for (const auto i : std::views::iota(0U, kCirclePointCount)) {
    const uint8_t angle = (i * kCircleStep);
    const auto [lx, ly] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), r);
    const auto [wx, wy] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), w);
    v[0] = {
        static_cast<VertexCoord>(center.x + lx - wx),
        static_cast<VertexCoord>(center.y + ly - wy),
    };
    v[1] = {
        static_cast<VertexCoord>(center.x + lx + wx),
        static_cast<VertexCoord>(center.y + ly + wy),
    };
    v += 2;
  }
}

} // namespace geometry

void GeomCircle(WindowPoint center, PixelCoord radius) {
  geometry::DrawCircle(Geometry(), center, radius);
}

void GeomCircleF(WindowPoint center, PixelCoord radius) {
  geometry::DrawFilledCircle(Geometry(), center, radius, false);
}
