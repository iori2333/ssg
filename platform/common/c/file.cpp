///
/// File I/O via std::filesystem
///

#include <SDL3/SDL_iostream.h>

#include "platform/file.h"

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
