///
/// Geometry drawing via SDL_Renderer
///

#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>

#include <SDL3/SDL_render.h>

#include "geometry.h"
#include "graphics_sdl_internal.h"

#include "gfx/core/pixelformat.h"
#include "sys/log.h"

namespace {

void DrawGeometry(TrianglePrimitive tp, VertexXySpan<> xys,
                  VertexRgbaSpan<> colors) {
#pragma warning(suppress : 26494) // type.5
  std::array<SDL_FPoint, kMaxTriangles> sdl_vertices{};
  std::array<SdlColor, kMaxTriangles> sdl_colors{};

  const auto vertex_count = xys.size();
  if (vertex_count < 3 || vertex_count > std::size(sdl_vertices)) {
    logging::Critical(kLogCat,
                      "Invalid vertex count for a triangle primitive: {}",
                      vertex_count);
    return;
  }
  const auto indices = kTriangleIndices[tp];
  const auto index_count = TriangleIndexCount(vertex_count);
  if (index_count > indices.size()) {
    logging::Critical(kLogCat,
                      "Invalid index count for a triangle primitive: {}",
                      index_count);
    return;
  }

  if (colors.size() != 1 && colors.size() != vertex_count) {
    logging::Critical(kLogCat,
                      "Triangle color count {} does not match vertex count {}",
                      colors.size(), vertex_count);
    return;
  }
  std::ranges::transform(colors, sdl_colors.begin(), [](const auto &color) {
    return SdlColor{.r = color.r, .g = color.g, .b = color.b, .a = color.a};
  });

  // Work around SDL's weird -0.5f offset... Default to a 1.0 scale so a
  // failed SDL_GetRenderScale call cannot leave NAN vertex offsets.
  float offset_x = 1.0F;
  float offset_y = 1.0F;
  SDL_GetRenderScale(RenderState().active_renderer, &offset_x, &offset_y);
  offset_x = (1.0F / (2.0F * offset_x));
  offset_y = (1.0F / (2.0F * offset_y));
  auto *sdl = sdl_vertices.data();
  for (const auto &game : xys) {
    *(sdl++) = {.x = (game.x + offset_x), .y = (game.y + offset_y)};
  }

  SDL_RenderGeometryRaw(
      RenderState().active_renderer, nullptr, &sdl_vertices[0].x,
      sizeof(SDL_FPoint), sdl_colors.data(),
      ((colors.size() == 1) ? 0 : sizeof(SdlColor)), nullptr, 0, vertex_count,
      indices.data(), index_count, sizeof(IndexType));
}

} // namespace

namespace geometry {

void SetColor(Rgb216 col) {
  const auto rgb = col.ToRgb();
  RenderState().color.r = rgb.r;
  RenderState().color.g = rgb.g;
  RenderState().color.b = rgb.b;
  SDL_SetRenderDrawColor(RenderState().active_renderer, RenderState().color.r,
                         RenderState().color.g, RenderState().color.b, 0xFF);
}

void SetAlphaNorm(uint8_t a) {
  RenderState().color.a = a;
  RenderState().alpha_mode = SDL_BLENDMODE_BLEND;
}

void SetAlphaOne() {
  RenderState().color.a = 0xFF;
  RenderState().alpha_mode = SDL_BLENDMODE_ADD;
}

void DrawLine(int x1, int y1, int x2, int y2) {
  SDL_RenderLine(RenderState().active_renderer, x1, y1, x2, y2);
}

void DrawBox(int x1, int y1, int x2, int y2) {
  const SDL_FRect rect = {
      .x = static_cast<float>(x1),
      .y = static_cast<float>(y1),
      .w = static_cast<float>(x2 - x1),
      .h = static_cast<float>(y2 - y1),
  };
  SDL_RenderFillRect(RenderState().active_renderer, &rect);
}

void DrawBoxA(int x1, int y1, int x2, int y2) {
  DrawWithAlpha([&] { DrawBox(x1, y1, x2, y2); });
}

void DrawLineStrip(VertexXySpan<> xys) {
  if (xys.size() > kMaxTriangles) {
    logging::Critical(kLogCat, "Too many points for a line strip: {}",
                      xys.size());
    return;
  }
  std::array<SDL_FPoint, kMaxTriangles> points{};
  std::ranges::transform(xys, points.begin(), [](const auto &point) {
    return SDL_FPoint{.x = point.x, .y = point.y};
  });
  SDL_RenderLines(RenderState().active_renderer, points.data(), xys.size());
}

void DrawTriangles(TrianglePrimitive tp, VertexXySpan<> xys,
                   VertexRgbaSpan<> colors) {
  if (colors.empty()) {
    const VertexRgba single = {RenderState().color.r, RenderState().color.g,
                               RenderState().color.b, 0xFF};
    DrawGeometry(tp, xys, std::span(&single, 1));
  } else {
    DrawGeometry(tp, xys, colors);
  }
}

void DrawTrianglesA(TrianglePrimitive tp, VertexXySpan<> xys,
                    VertexRgbaSpan<> colors) {
  DrawWithAlpha([&] {
    if (colors.empty()) {
      const VertexRgba single = {RenderState().color.r, RenderState().color.g,
                                 RenderState().color.b, RenderState().color.a};
      DrawGeometry(tp, xys, std::span(&single, 1));
    } else {
      DrawGeometry(tp, xys, colors);
    }
  });
}

void DrawGrdLineEx(int x, int y1, Rgb c1, int y2, Rgb c2) {
  const auto c1a = c1.WithAlpha(0xFF);
  const auto c2a = c2.WithAlpha(0xFF);
  const std::array<VertexXy, 4> xys = {
      VertexXy{static_cast<float>(x + 0), static_cast<float>(y1)},
      VertexXy{static_cast<float>(x + 0), static_cast<float>(y2)},
      VertexXy{static_cast<float>(x + 1), static_cast<float>(y1)},
      VertexXy{static_cast<float>(x + 1), static_cast<float>(y2)},
  };
  const std::array<VertexRgba, 4> colors = {c1a, c2a, c1a, c2a};
  DrawGeometry(TrianglePrimitive::Strip, xys, colors);
}

} // namespace geometry