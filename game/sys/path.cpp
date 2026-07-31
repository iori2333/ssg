///
/// Common paths and path manipulation via SDL
///

#include <string_view>

#include <SDL3/SDL_filesystem.h>

#include "path.h"

static std::string_view PathDataView;

std::string_view PathForData(void) {
  if (PathDataView.data() != nullptr) {
    return PathDataView;
  }
  const auto *path = SDL_GetBasePath();
  PathDataView = path ? std::string_view(path) : std::string_view{};
  return PathDataView;
}
