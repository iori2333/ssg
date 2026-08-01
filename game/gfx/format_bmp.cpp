///
/// .BMP file format
///

#include <cassert>

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_pixels.h>

#include "format_bmp.h"

#include "util/byte_io.h"

namespace {

bool WriteExact(SDL_IOStream *stream, const void *data, size_t size) {
  return SDL_WriteIO(stream, data, size) == size;
}

} // namespace

uint16_t BmpPaletteSizeFromBpp(uint8_t bpp) {
  const auto ret = [bpp]() -> uint16_t {
    if (bpp <= 4) {
      return (1 << 4);
    } else if (bpp <= 8) {
      return (1 << 8);
    }
    return 0;
  }();
  assert(ret <= kBmpPaletteSizeMax);
  return ret;
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

  // It makes sense to run this function on non-.BMP files when doing
  // automatic file type detection, which should have failed in the two
  // checks above. But if we got here, we expect this to be a valid .BMP,
  // and therefore assert() that it is.
  const auto header_info = reader.ReadObject<BmpInfoHeader>();
  if (!header_info) {
    assert(!"Not a .BMP file?");
    return std::nullopt;
  }

  const auto palette_size =
      BmpPaletteSizeFromBpp(header_info->biPlanes * header_info->biBitCount);
  if (!reader.ReadBytes(palette_size * sizeof(Bgra))) {
    assert(!"Needs a palette, but doesn't contain a full one?");
    return std::nullopt;
  }

  // [header_info.biSizeImage] can be 0, so we have to manually calculate the
  // actual size allocated by CreateDIBSection() by DWORD-aligning the row
  // stride. We can't just take everything from [image] to the end of the
  // buffer because nothing prevents the file from being larger than what
  // CreateDIBSection() allocated. This actually happens with File 22 in
  // GRAPH.DAT (Reimu's faceset).
  const size_t size = (header_info->Stride() * header_info->biHeight);
  if (!reader.Seek(header_file->bfOffBits) || !reader.ReadBytes(size)) {
    assert(!"Does not contain all pixels?");
    return std::nullopt;
  }

  return BmpOwned{std::move(buffer), *header_info, header_file->bfOffBits,
                  size};
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

bool BmpSave(SDL_IOStream *stream, PixelSize size, uint16_t planes,
             uint16_t bpp, std::span<Bgra> palette,
             std::span<const std::byte> pixels) {
  assert(pixels.size_bytes() <= std::numeric_limits<uint32_t>::max());
  assert(palette.size() <= std::numeric_limits<uint32_t>::max());
  const BmpInfoHeader header_info = {
      .biSize = sizeof(BmpInfoHeader),
      .biWidth = size.w,
      .biHeight = size.h,
      .biPlanes = planes,
      .biBitCount = bpp,
      .biCompression = 0, // BI_RGB
      .biSizeImage = static_cast<uint32_t>(pixels.size_bytes()),
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
  return (stream && WriteExact(stream, &header_file, sizeof(header_file)) &&
          WriteExact(stream, &header_info, sizeof(header_info)) &&
          WriteExact(stream, palette.data(), palette.size_bytes()) &&
          WriteExact(stream, pixels.data(), pixels.size_bytes()));
}
