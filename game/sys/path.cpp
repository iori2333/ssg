///
/// Common paths and path manipulation via SDL
///

#include <string_view>

#include <SDL3/SDL_filesystem.h>

#include "path.h"

namespace {
std::string_view PathDataView;
} // namespace

std::string_view PathForData() {
  if (PathDataView.data() != nullptr) {
    return PathDataView;
  }
  const auto *path = SDL_GetBasePath();
  PathDataView =
      (path != nullptr) ? std::string_view(path) : std::string_view{};
  return PathDataView;
}
