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
constexpr size_t CIRCLE_POINTS = 33;

void ApproximateCircle(std::span<VERTEX_XY, CIRCLE_POINTS> ret,
                       WINDOW_POINT center, PIXEL_COORD radius);

void ApproximateFatCircle(std::span<VERTEX_XY, (CIRCLE_POINTS * 2)> ret,
                          WINDOW_POINT center, PIXEL_COORD r, PIXEL_COORD w);
// -----------------

// Implementations
// ---------------

inline void DrawCircle(GraphicsGeometry &graphics, WINDOW_POINT center,
                       PIXEL_COORD radius) {
  std::array<VERTEX_XY, CIRCLE_POINTS> xys{};
  ApproximateCircle(xys, center, radius);
  graphics.DrawLineStrip(xys);
}

inline void DrawFilledCircle(GraphicsGeometry &graphics, WINDOW_POINT center,
                             PIXEL_COORD radius, bool alpha) {
  std::array<VERTEX_XY, (1 + CIRCLE_POINTS)> xys{};
  xys[0].x = static_cast<VERTEX_COORD>(center.x);
  xys[0].y = static_cast<VERTEX_COORD>(center.y);
  ApproximateCircle(std::span(xys).template subspan<1, CIRCLE_POINTS>(), center,
                    radius);
  if (alpha) {
    graphics.DrawTrianglesA(TRIANGLE_PRIMITIVE::FAN, xys);
  } else {
    graphics.DrawTriangles(TRIANGLE_PRIMITIVE::FAN, xys);
  }
}

inline void DrawAlphaFatCircle(GraphicsGeometry &graphics, WINDOW_POINT center,
                               PIXEL_COORD r, PIXEL_COORD w) {
  // When it becomes a regular circle
  if (w >= r) {
    geometry::DrawFilledCircle(graphics, center, (r + w), true);
  }
  std::array<VERTEX_XY, (CIRCLE_POINTS * 2)> xys{};
  ApproximateFatCircle(xys, center, r, w);
  graphics.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, xys);
}

inline void DrawGradientRect(GraphicsGeometry &graphics,
                             std::span<const VERTEX_XY, 4> p, RGBA col_edge,
                             bool alpha) {
  const RGBA col_center = {.r = 255, .g = 255, .b = 255, .a = col_edge.a};

  // Use an explicit integer division for a pixel-perfect match of the
  // original look, even if VERTEX_XY is a floating-point type.
  const std::array<VERTEX_XY, 6> xys = {
      p[3], p[2], (p[0] + p[3]).DivInt(2), (p[1] + p[2]).DivInt(2), p[0], p[1],
  };
  const std::array<VERTEX_RGBA, 6> colors = {
      col_edge, col_edge, col_center, col_center, col_edge, col_edge,
  };
  if (alpha) {
    graphics.DrawTrianglesA(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
  } else {
    graphics.DrawTriangles(TRIANGLE_PRIMITIVE::STRIP, xys, colors);
  }
}
// ---------------

} // namespace geometry

// Draw calls
// ----------

// Circle outline
void GeomCircle(WINDOW_POINT center, PIXEL_COORD radius);

// Filled circle
void GeomCircleF(WINDOW_POINT center, PIXEL_COORD radius);

// Alpha-blended fat circle
inline void GeomFatCircleA(GraphicsGeometry &graphics, WINDOW_POINT center,
                           PIXEL_COORD r, PIXEL_COORD w) {
  geometry::DrawAlphaFatCircle(graphics, center, r, w);
}

// Gradient rectangle (can be diagonal)
inline void GeomGrdRect(GraphicsGeometry &graphics,
                        std::span<const VERTEX_XY, 4> points, RGB col_edge) {
  geometry::DrawGradientRect(graphics, points, col_edge.WithAlpha(0xFF), false);
}

// Gradient rectangle (can be diagonal + alpha)
inline void GeomGrdRectA(GraphicsGeometry &graphics,
                         std::span<const VERTEX_XY, 4> points, RGBA col_edge) {
  geometry::DrawGradientRect(graphics, points, col_edge, true);
}
// ----------
