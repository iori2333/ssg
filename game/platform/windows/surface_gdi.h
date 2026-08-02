///
/// 2D surfaces via GDI bitmaps
///

#pragma once

#include <windows.h>
// Only required for the HBITMAP type, which is basically void*.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "gfx/coords.h"

struct Surface {
  PixelSize size = {.w = 0, .h = 0};
};

struct BmpOwned;
struct SDL_IOStream;

class SurfaceGdi : public Surface {
public:
  // Required for unselecting [img] prior to deleting it. Could have probably
  // been `static`, but let's keep it correct for now.
  HGDIOBJ stock_img = nullptr;

  // Source bitmap data, if any.
  HBITMAP img = nullptr;

  // Always has any valid [img] selected into it.
  HDC dc;

  SurfaceGdi() noexcept;
  SurfaceGdi(const SurfaceGdi &) = delete;
  SurfaceGdi &operator=(const SurfaceGdi &) = delete;
  SurfaceGdi(SurfaceGdi &&) = delete;
  SurfaceGdi &operator=(SurfaceGdi &&) = delete;
  ~SurfaceGdi();

  // Calls Delete() and reinitializes [img].
  bool Load(const BmpOwned &bmp);

  // Saves [img] as a .BMP file to the given stream.
  bool Save(SDL_IOStream * /*stream*/) const;

  void Delete() noexcept;
};
