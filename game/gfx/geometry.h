///
/// Geometry - Backend-independent geometry rendering functions
///

#pragma once

#include "gfx/graphics_backend.h"

namespace geometry {

// Vertex generators
// -----------------

// pbg's Direct3D backend approximated circles as 32-sided polygons. The first
// point is duplicated at the end to simplify index buffer generation.
constexpr size_t kCirclePointCount = 33;

void ApproximateCircle(std::span<VertexXy, kCirclePointCount> ret,
                       WindowPoint center, PixelCoord radius);

void ApproximateFatCircle(std::span<VertexXy, (kCirclePointCount * 2)> ret,
                          WindowPoint center, PixelCoord r, PixelCoord w);
// -----------------

// Implementations
// ---------------

inline void DrawCircle(GraphicsGeometry &graphics, WindowPoint center,
                       PixelCoord radius) {
  std::array<VertexXy, kCirclePointCount> xys{};
  ApproximateCircle(xys, center, radius);
  graphics.DrawLineStrip(xys);
}

inline void DrawFilledCircle(GraphicsGeometry &graphics, WindowPoint center,
                             PixelCoord radius, bool alpha) {
  std::array<VertexXy, (1 + kCirclePointCount)> xys{};
  xys[0].x = static_cast<VertexCoord>(center.x);
  xys[0].y = static_cast<VertexCoord>(center.y);
  ApproximateCircle(std::span(xys).template subspan<1, kCirclePointCount>(),
                    center, radius);
  if (alpha) {
    graphics.DrawTrianglesA(TrianglePrimitive::Fan, xys);
  } else {
    graphics.DrawTriangles(TrianglePrimitive::Fan, xys);
  }
}

inline void DrawAlphaFatCircle(GraphicsGeometry &graphics, WindowPoint center,
                               PixelCoord r, PixelCoord w) {
  // When it becomes a regular circle
  if (w >= r) {
    geometry::DrawFilledCircle(graphics, center, (r + w), true);
  }
  std::array<VertexXy, (kCirclePointCount * 2)> xys{};
  ApproximateFatCircle(xys, center, r, w);
  graphics.DrawTrianglesA(TrianglePrimitive::Strip, xys);
}

inline void DrawGradientRect(GraphicsGeometry &graphics,
                             std::span<const VertexXy, 4> p, Rgba col_edge,
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
    graphics.DrawTrianglesA(TrianglePrimitive::Strip, xys, colors);
  } else {
    graphics.DrawTriangles(TrianglePrimitive::Strip, xys, colors);
  }
}
// ---------------

} // namespace geometry

// Draw calls
// ----------

// Circle outline
void GeomCircle(WindowPoint center, PixelCoord radius);

// Filled circle
void GeomCircleF(WindowPoint center, PixelCoord radius);

// Alpha-blended fat circle
inline void GeomFatCircleA(GraphicsGeometry &graphics, WindowPoint center,
                           PixelCoord r, PixelCoord w) {
  geometry::DrawAlphaFatCircle(graphics, center, r, w);
}

// Gradient rectangle (can be diagonal)
inline void GeomGrdRect(GraphicsGeometry &graphics,
                        std::span<const VertexXy, 4> points, Rgb col_edge) {
  geometry::DrawGradientRect(graphics, points, col_edge.WithAlpha(0xFF), false);
}

// Gradient rectangle (can be diagonal + alpha)
inline void GeomGrdRectA(GraphicsGeometry &graphics,
                         std::span<const VertexXy, 4> points, Rgba col_edge) {
  geometry::DrawGradientRect(graphics, points, col_edge, true);
}
// ----------
