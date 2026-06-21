///
/// File I/O
///

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

#include "game/buffer.h"
#include "game/file.h"

std::optional<FILE_TIMESTAMPS> File_TimestampsGet(const char *fn) {
  std::error_code ec;
  auto time = std::filesystem::last_write_time(fn, ec);
  if (ec) {
    return std::nullopt;
  }
  return time;
}

bool File_CloseWithTimestamps(
    SDL_IOStream *&&context, const char *path,
    std::optional<FILE_TIMESTAMPS> maybe_time) {
  const bool ret = SDL_CloseIO(context);
  if (maybe_time) {
    std::error_code ec;
    std::filesystem::last_write_time(path, *maybe_time, ec);
  }
  return ret;
}

BYTE_BUFFER_OWNED SDL_LoadFile(const char *file) {
  auto *f = SDL_IOFromFile(file, "rb");
  if (!f) {
    return {};
  }
  return SDL_LoadFile_IO(f, true);
}

BYTE_BUFFER_OWNED SDL_LoadFile_IO(SDL_IOStream *src, bool closeio) {
  size_t size;
  auto *buf = SDL_LoadFile_IO(src, &size, true);
  if (!buf) {
    return {};
  }
  return {std::move(buf), size};
}

bool SDL_MustReadIO(SDL_IOStream *context, void *ptr, size_t size) {
  return (SDL_ReadIO(context, ptr, size) == size);
}

bool SDL_MustWriteIO(SDL_IOStream *context, const void *ptr, size_t size) {
  return (SDL_WriteIO(context, ptr, size) == size);
}
