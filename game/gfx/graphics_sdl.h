///
/// Graphics via SDL_Renderer
///

#pragma once

#include "graphics_backend.h"

class GraphicsGeometry {
public:
  void SetColor(Rgb216 col);
  void SetAlphaNorm(uint8_t a);
  void SetAlphaOne();
  void DrawLine(int x1, int y1, int x2, int y2);
  void DrawBox(int x1, int y1, int x2, int y2);
  void DrawBoxA(int x1, int y1, int x2, int y2);
  void DrawTriangleFan(VertexXySpan<>);
  // ----------

  // Poly methods
  // ------------

  void DrawLineStrip(VertexXySpan<>);
  void DrawTriangles(TrianglePrimitive, VertexXySpan<>,
                     VertexRgbaSpan<> colors = {});
  void DrawTrianglesA(TrianglePrimitive, VertexXySpan<>,
                      VertexRgbaSpan<> colors = {});
  void DrawGrdLineEx(int x, int y1, Rgb c1, int y2, Rgb c2);
};
