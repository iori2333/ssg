///
/// .BMP file format
///

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_pixels.h>

#include "format_bmp.h"

#include "gfx/core/coords.h"
#include "gfx/core/pixelformat.h"
#include "util/byte_io.h"

namespace {

bool WriteExact(SDL_IOStream *stream, const void *data, size_t size) {
  return SDL_WriteIO(stream, data, size) == size;
}

} // namespace

uint16_t BmpPaletteSizeFromBpp(uint8_t bpp) {
  return [bpp]() -> uint16_t {
    if (bpp <= 4) {
      return (1 << 4);
    }
    if (bpp <= 8) {
      return (1 << 8);
    }
    return 0;
  }();
}

std::optional<BmpOwned> BmpLoad(std::vector<uint8_t> buffer) {
  if (buffer.empty()) {
    return std::nullopt;
  }

  util::ByteReader reader{buffer};
  const auto header_file = reader.ReadObject<BmpFileHeader>();
  if (!header_file) {
    return std::nullopt;
  }

  if (header_file->bfType != 0x4D42) { // "BM"
    return std::nullopt;
  }

  const auto header_info = reader.ReadObject<BmpInfoHeader>();
  if (!header_info) {
    return std::nullopt;
  }

  const auto palette_size =
      BmpPaletteSizeFromBpp(header_info->biPlanes * header_info->biBitCount);
  if (!reader.ReadBytes(palette_size * sizeof(Bgra))) {
    return std::nullopt;
  }

  // [header_info.biSizeImage] can be 0, so we have to manually calculate the
  // actual size allocated by CreateDIBSection() by DWORD-aligning the row
  // stride. We can't just take everything from [image] to the end of the
  // buffer because nothing prevents the file from being larger than what
  // CreateDIBSection() allocated. This actually happens with File 22 in
  // GRAPH.DAT (Reimu's faceset).
  const size_t size =
      (static_cast<size_t>(header_info->Stride()) * header_info->biHeight);
  if (!reader.Seek(header_file->bfOffBits) || !reader.ReadBytes(size)) {
    return std::nullopt;
  }

  return BmpOwned{.buffer = std::move(buffer),
                  .info = *header_info,
                  .pixel_offset = header_file->bfOffBits,
                  .pixel_size = size};
}

bool BmpSaveSupports(SDL_PixelFormat format) {
  switch (format) {
  case SDL_PIXELFORMAT_INDEX8:
  case SDL_PIXELFORMAT_XRGB1555:
  case SDL_PIXELFORMAT_XRGB8888:
  case SDL_PIXELFORMAT_ARGB8888:
    return true;
  default:
    return false;
  }
}

bool BmpSave(SDL_IOStream *stream, PixelPoint size, uint16_t planes,
             uint16_t bpp, std::span<Bgra> palette,
             std::span<const uint8_t> pixels) {
  if (pixels.size() > std::numeric_limits<uint32_t>::max() ||
      palette.size() > std::numeric_limits<uint32_t>::max()) {
    return false;
  }
  const BmpInfoHeader header_info = {
      .biSize = sizeof(BmpInfoHeader),
      .biWidth = size.x,
      .biHeight = size.y,
      .biPlanes = planes,
      .biBitCount = bpp,
      .biCompression = 0, // BI_RGB
      .biSizeImage = static_cast<uint32_t>(pixels.size()),
      .biClrUsed = static_cast<uint32_t>(palette.size()),
  };
  const uint32_t pixel_offset =
      (sizeof(BmpFileHeader) + header_info.biSize + palette.size_bytes());
  const BmpFileHeader header_file = {
      .bfType = 0x4D42, // "BM"
      .bfSize = pixel_offset,
      .bfReserved1 = 0,
      .bfReserved2 = 0,
      .bfOffBits = pixel_offset,
  };
  return ((stream != nullptr) &&
          WriteExact(stream, &header_file, sizeof(header_file)) &&
          WriteExact(stream, &header_info, sizeof(header_info)) &&
          WriteExact(stream, palette.data(), palette.size_bytes()) &&
          WriteExact(stream, pixels.data(), pixels.size()));
}
