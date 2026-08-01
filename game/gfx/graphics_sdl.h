///
/// Graphics via SDL_Renderer
///

#pragma once

#include "graphics_backend.h"

class GraphicsGeometry {
public:
  void SetColor(RGB216 col);
  void SetAlphaNorm(uint8_t a);
  void SetAlphaOne();
  void DrawLine(int x1, int y1, int x2, int y2);
  void DrawBox(int x1, int y1, int x2, int y2);
  void DrawBoxA(int x1, int y1, int x2, int y2);
  void DrawTriangleFan(VERTEX_XY_SPAN<>);
  // ----------

  // Poly methods
  // ------------

  void DrawLineStrip(VERTEX_XY_SPAN<>);
  void DrawTriangles(TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>,
                     VERTEX_RGBA_SPAN<> colors = {});
  void DrawTrianglesA(TRIANGLE_PRIMITIVE, VERTEX_XY_SPAN<>,
                      VERTEX_RGBA_SPAN<> colors = {});
  void DrawGrdLineEx(int x, int y1, RGB c1, int y2, RGB c2);
};
