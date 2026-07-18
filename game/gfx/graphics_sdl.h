///
/// Graphics via SDL_Renderer
///

#pragma once

#include "graphics_backend.h"

class GRAPHICS_GEOMETRY_SDL {
public:
  void Lock(void);
  void Unlock(void);
  void SetColor(RGB216 col);
  void SetAlphaNorm(uint8_t a);
  void SetAlphaOne(void);
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
  // ------------

  // Framebuffer methods
  // -------------------
  // Just required to satisfy the framebuffer concept, since our GrpGeom_FB()
  // also returns a pointer to this class.

  void DrawPoint(WINDOW_POINT p);
  void DrawHLine(int x1, int x2, int y);
  // -------------------
};
