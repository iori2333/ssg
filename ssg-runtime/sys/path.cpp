///
/// Common paths and path manipulation via SDL
///

#include <string>
#include <string_view>

#include <SDL3/SDL_filesystem.h>

#include "path.h"

namespace {

// Copies the base path into owned storage so the returned view is stable.
// NOTE: SDL_GetBasePath() returns a process-lifetime cached pointer in this
// SDL3 version (see SDL_filesystem.c) and must NOT be released with SDL_free.
std::string OwnedBasePath() {
  const auto *path = SDL_GetBasePath();
  return path != nullptr ? std::string(path) : std::string{};
}

} // namespace

std::string_view PathForData() {
  static const std::string base = OwnedBasePath();
  return base;
}