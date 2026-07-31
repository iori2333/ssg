///
/// Geometry - Backend-independent geometry rendering functions
///

#include "geometry.h"

#include "util/math_utils.h"

namespace Geometry {

constexpr uint8_t CIRCLE_STEP = (0x100 / (CIRCLE_POINTS - 1));

void ApproximateCircle(std::span<VERTEX_XY, CIRCLE_POINTS> ret,
                       WINDOW_POINT center, PIXEL_COORD radius) {
  auto i = 0;
  for (auto &v : ret) {
    const uint8_t angle = (i++ * CIRCLE_STEP);
    const auto offset =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), radius);
    v.x = static_cast<VERTEX_COORD>(center.x + offset.x);
    v.y = static_cast<VERTEX_COORD>(center.y + offset.y);
  }
}

void ApproximateFatCircle(std::span<VERTEX_XY, (CIRCLE_POINTS * 2)> ret,
                          WINDOW_POINT center, PIXEL_COORD r, PIXEL_COORD w) {
  auto v = ret.begin();
  for (const auto i : std::views::iota(0U, CIRCLE_POINTS)) {
    const uint8_t angle = (i * CIRCLE_STEP);
    const auto [lx, ly] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), r);
    const auto [wx, wy] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), w);
    v[0] = {
        static_cast<VERTEX_COORD>(center.x + lx - wx),
        static_cast<VERTEX_COORD>(center.y + ly - wy),
    };
    v[1] = {
        static_cast<VERTEX_COORD>(center.x + lx + wx),
        static_cast<VERTEX_COORD>(center.y + ly + wy),
    };
    v += 2;
  }
}

} // namespace Geometry

void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius) {
  if (auto *gp = GrpGeom_Poly()) {
    Geometry::Circle_Approximated(*gp, center, radius);
  } else if (auto *gf = GrpGeom_FB()) {
    Geometry::Circle_Exact(*gf, center, radius);
  }
}

void GeomCircleF(WINDOW_POINT center, PIXEL_COORD radius) {
  if (auto *gp = GrpGeom_Poly()) {
    Geometry::CircleF_Approximated(*gp, center, radius, false);
  } else if (auto *gf = GrpGeom_FB()) {
    Geometry::CircleF_Exact(*gf, center, radius);
  }
}
