///
/// .BMP file format
///

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "coords.h"
#include "pixelformat.h"

#include "util/endian.h"

struct SDL_IOStream;

// Platform-independent .BMP header types
// --------------------------------------
// Yup, these actually need to be packed.
#pragma pack(push, 1)

// Same as the standard Win32 BITMAPFILEHEADER structure, renamed to avoid
// collisions.
struct BMP_FILEHEADER {
  util::LittleEndian<uint16_t> bfType;
  util::LittleEndian<uint32_t> bfSize;
  util::LittleEndian<uint16_t> bfReserved1;
  util::LittleEndian<uint16_t> bfReserved2;
  util::LittleEndian<uint32_t> bfOffBits;
};

// Same as the standard Win32 BITMAPINFOHEADER structure, renamed to avoid
// collisions.
struct BMP_INFOHEADER {
  util::LittleEndian<uint32_t> biSize;
  util::LittleEndian<int32_t> biWidth;
  util::LittleEndian<int32_t> biHeight;
  util::LittleEndian<uint16_t> biPlanes;
  util::LittleEndian<uint16_t> biBitCount;
  util::LittleEndian<uint32_t> biCompression;
  util::LittleEndian<uint32_t> biSizeImage;
  util::LittleEndian<int32_t> biXPelsPerMeter;
  util::LittleEndian<int32_t> biYPelsPerMeter;
  util::LittleEndian<uint32_t> biClrUsed;
  util::LittleEndian<uint32_t> biClrImportant;

  uint32_t Stride() const {
    return ((((biWidth * biBitCount) + 31u) & ~31) / 8u);
  }
};

#pragma pack(pop)
// --------------------------------------

// A validated .BMP buffer with its decoded header and pixel range.
struct BMP_OWNED {
  std::vector<uint8_t> buffer;
  BMP_INFOHEADER info;
  size_t pixel_offset;
  size_t pixel_size;

  [[nodiscard]] std::span<std::byte> Pixels() {
    return {reinterpret_cast<std::byte *>(buffer.data() + pixel_offset),
            pixel_size};
  }
};

// Can be safely used for static allocations.
constexpr uint16_t BMP_PALETTE_SIZE_MAX = 256;

// Returns a value between 0 and [BMP_PALETTE_SIZE_MAX].
uint16_t BMPPaletteSizeFromBPP(uint8_t bpp);

std::optional<BMP_OWNED> BMPLoad(std::vector<uint8_t> buffer);

#ifndef SDL_pixels_h_
enum SDL_PixelFormat : uint32_t;
#endif

// Returns `true` if BMPSave() supports the given [format].
bool BMPSaveSupports(SDL_PixelFormat format);

bool BMPSave(SDL_IOStream *stream, PIXEL_SIZE size, uint16_t planes,
             uint16_t bpp, std::span<BGRA> palette,
             std::span<const std::byte> pixels);
