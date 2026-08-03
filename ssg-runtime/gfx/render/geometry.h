///
/// Geometry - Backend-independent geometry rendering functions
///

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>

#include "gfx/core/coords.h"
#include "gfx/core/pixelformat.h"
#include "util/math_utils.h"

// Geometry vertex types
// ---------------------

struct VertexXy {
  float x{};
  float y{};

  [[nodiscard]] constexpr VertexXy DivInt(int scalar) const {
    return {
        .x = static_cast<float>(static_cast<int>(x) / scalar),
        .y = static_cast<float>(static_cast<int>(y) / scalar),
    };
  }

  constexpr VertexXy operator+(const VertexXy &other) const {
    return {(x + other.x), (y + other.y)};
  }
};

struct VertexRgba {
  float r;
  float g;
  float b;
  float a;

  VertexRgba() = default;
  VertexRgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : r(r / 255.0F), g(g / 255.0F), b(b / 255.0F), a(a / 255.0F) {}
  VertexRgba(const Rgba &o)
      : r(o.r / 255.0F), g(o.g / 255.0F), b(o.b / 255.0F), a(o.a / 255.0F) {}
};

template <size_t N = std::dynamic_extent>
using VertexXySpan = std::span<const VertexXy, N>;
template <size_t N = std::dynamic_extent>
using VertexRgbaSpan = std::span<const VertexRgba, N>;

enum class TrianglePrimitive : uint8_t { Fan, Strip, Count };
// ---------------------

namespace geometry {

// Draw state and primitive entry points
// ------------------------------------

void SetColor(Rgb216 col);
void SetAlphaNorm(uint8_t a);
void SetAlphaOne();
void DrawLine(int x1, int y1, int x2, int y2);
void DrawBox(int x1, int y1, int x2, int y2);
void DrawBoxA(int x1, int y1, int x2, int y2);
void DrawLineStrip(VertexXySpan<> xys);
void DrawTriangles(TrianglePrimitive tp, VertexXySpan<> xys,
                   VertexRgbaSpan<> colors = {});
void DrawTrianglesA(TrianglePrimitive tp, VertexXySpan<> xys,
                    VertexRgbaSpan<> colors = {});
void DrawGrdLineEx(int x, int y1, Rgb c1, int y2, Rgb c2);
// ------------------------------------

// Vertex generators
// -----------------

// pbg's Direct3D backend approximated circles as 32-sided polygons. The first
// point is duplicated at the end to simplify index buffer generation.
constexpr size_t kCirclePointCount = 33;

constexpr uint8_t kCircleStep = (0x100 / (kCirclePointCount - 1));

inline void ApproximateCircle(std::span<VertexXy, kCirclePointCount> ret,
                              PixelPoint center, int radius) {
  auto i = 0;
  for (auto &v : ret) {
    const uint8_t angle = (i++ * kCircleStep);
    const auto offset =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), radius);
    v.x = static_cast<float>(center.x + offset.x);
    v.y = static_cast<float>(center.y + offset.y);
  }
}

inline void
ApproximateFatCircle(std::span<VertexXy, (kCirclePointCount * 2)> ret,
                     PixelPoint center, int r, int w) {
  auto v = ret.begin();
  for (const auto i : std::views::iota(0U, kCirclePointCount)) {
    const uint8_t angle = (i * kCircleStep);
    const auto [lx, ly] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), r);
    const auto [wx, wy] =
        math::RoundedPolarVector(math::AngleFromLegacy(angle), w);
    v[0] = {
        static_cast<float>(center.x + lx - wx),
        static_cast<float>(center.y + ly - wy),
    };
    v[1] = {
        static_cast<float>(center.x + lx + wx),
        static_cast<float>(center.y + ly + wy),
    };
    v += 2;
  }
}
// -----------------

// Implementations
// ---------------

inline void DrawCircle(PixelPoint center, int radius) {
  std::array<VertexXy, kCirclePointCount> xys{};
  ApproximateCircle(xys, center, radius);
  geometry::DrawLineStrip(xys);
}

inline void DrawFilledCircle(PixelPoint center, int radius, bool alpha) {
  std::array<VertexXy, (1 + kCirclePointCount)> xys{};
  xys[0].x = static_cast<float>(center.x);
  xys[0].y = static_cast<float>(center.y);
  ApproximateCircle(std::span(xys).template subspan<1, kCirclePointCount>(),
                    center, radius);
  if (alpha) {
    geometry::DrawTrianglesA(TrianglePrimitive::Fan, xys);
  } else {
    geometry::DrawTriangles(TrianglePrimitive::Fan, xys);
  }
}

inline void DrawAlphaFatCircle(PixelPoint center, int r, int w) {
  // When it becomes a regular circle
  if (w >= r) {
    geometry::DrawFilledCircle(center, (r + w), true);
    return;
  }
  std::array<VertexXy, (kCirclePointCount * 2)> xys{};
  ApproximateFatCircle(xys, center, r, w);
  geometry::DrawTrianglesA(TrianglePrimitive::Strip, xys);
}

inline void DrawGradientRect(std::span<const VertexXy, 4> p, Rgba col_edge,
                             bool alpha) {
  const Rgba col_center = {.r = 255, .g = 255, .b = 255, .a = col_edge.a};

  // Use an explicit integer division for a pixel-perfect match of the
  // original look, even if VertexXy is a floating-point type.
  const std::array<VertexXy, 6> xys = {
      p[3], p[2], (p[0] + p[3]).DivInt(2), (p[1] + p[2]).DivInt(2), p[0], p[1],
  };
  const std::array<VertexRgba, 6> colors = {
      col_edge, col_edge, col_center, col_center, col_edge, col_edge,
  };
  if (alpha) {
    geometry::DrawTrianglesA(TrianglePrimitive::Strip, xys, colors);
  } else {
    geometry::DrawTriangles(TrianglePrimitive::Strip, xys, colors);
  }
}
// ---------------

} // namespace geometry
