/*
 *   SDL wrappers for file I/O
 *
 */

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>

#include "platform/file.h"

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
