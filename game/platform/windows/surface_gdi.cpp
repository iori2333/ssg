///
/// 2D surfaces via GDI bitmaps
///

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// NOLINTBEGIN(misc-include-cleaner) - Windows SDK headers require windows.h.
#include <windows.h>

#include "surface_gdi.h"

#include "gfx/coords.h"
#include "gfx/format_bmp.h"
#include "gfx/pixelformat.h"

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

SurfaceGdi::SurfaceGdi() noexcept : dc(CreateCompatibleDC(nullptr)) {}

SurfaceGdi::~SurfaceGdi() {
  Delete();
  DeleteDC(dc);
}

static_assert(sizeof(RGBQUAD) == sizeof(Bgra));
static_assert(offsetof(RGBQUAD, rgbBlue) == offsetof(Bgra, b));
static_assert(offsetof(RGBQUAD, rgbGreen) == offsetof(Bgra, g));
static_assert(offsetof(RGBQUAD, rgbRed) == offsetof(Bgra, r));
static_assert(offsetof(RGBQUAD, rgbReserved) == offsetof(Bgra, a));

bool SurfaceGdi::Save(SDL_IOStream *stream) const {
  if (stream == nullptr) {
    return false;
  }

  DIBSECTION dib;
  if (!GetObject(img, sizeof(DIBSECTION), &dib)) {
    return false;
  }

  if ((dib.dsBm.bmHeight < 0) || (dib.dsBm.bmWidthBytes < 0)) {
    assert(!"Negative height or stride?");
    return false;
  }

  const auto bpp = (dib.dsBm.bmPlanes * dib.dsBm.bmBitsPixel);
  const auto palette_size = BmpPaletteSizeFromBpp(bpp);

  if (dib.dsBm.bmBits == nullptr) {
    // This is a DDB, not a DIB, which means that the BITMAPINFOHEADER
    // structure is invalid. Reconstruct it from what we have.
    // Adapted from
    //
    // 	https://learn.microsoft.com/en-us/windows/win32/gdi/storing-an-image
    const PixelSize size = {.w = dib.dsBm.bmWidth, .h = dib.dsBm.bmHeight};

    // GetDIBits() will write the color table after the BITMAPINFOHEADER
    // structure.
    std::vector<uint8_t> info_buf(sizeof(BITMAPINFOHEADER) +
                                  (sizeof(RGBQUAD) * palette_size));

    auto *info = reinterpret_cast<BmpInfoHeader *>(info_buf.data());
    const std::span<Bgra> palette = {reinterpret_cast<Bgra *>(&info[1]),
                                     palette_size};

    // According to MSDN, we explicitly only need the first 6 members.
    // GetDIBits() overwrites the rest anyway.
    *info = {
        .biSize = sizeof(dib.dsBmih),
        .biWidth = size.w,
        .biHeight = size.h,
        .biPlanes = dib.dsBm.bmPlanes,
        .biBitCount = dib.dsBm.bmBitsPixel,
        .biCompression = BI_RGB,
    };
    info->biSizeImage = (info->Stride() * info->biHeight);

    std::vector<uint8_t> pixels(info->biSizeImage);
    auto *bmi = reinterpret_cast<BITMAPINFO *>(info);
    if (GetDIBits(dc, img, 0, size.h, pixels.data(), bmi, DIB_RGB_COLORS) ==
        0) {
      assert(!"GetDIBits failed");
      return false;
    }
    return BmpSave(stream, size, info->biPlanes, info->biBitCount, palette,
                   pixels);
  }

  // For DIBs, we get direct access to the pixel data.
  std::array<Bgra, kBmpPaletteSizeMax> bgra{};
  std::span<Bgra> palette = {};
  const auto color_table_ret = GetDIBColorTable(
      dc, 0, palette_size, reinterpret_cast<RGBQUAD *>(bgra.data()));
  if (color_table_ret == palette_size) {
    palette = bgra;
  }
  const std::span<const uint8_t> pixels = {
      static_cast<const uint8_t *>(dib.dsBm.bmBits),

      // Negative values are checked above.
      static_cast<size_t>(dib.dsBm.bmWidthBytes * dib.dsBm.bmHeight)};
  return BmpSave(stream, {.w = dib.dsBmih.biWidth, .h = dib.dsBmih.biHeight},
                 dib.dsBmih.biPlanes, dib.dsBmih.biBitCount, palette, pixels);
}

void SurfaceGdi::Delete() noexcept {
  if (img != nullptr) {
    SelectObject(dc, stock_img);
    DeleteObject(img);
    img = nullptr;
  }
}
// NOLINTEND(misc-include-cleaner)
