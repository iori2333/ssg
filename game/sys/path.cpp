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
  PathDataView = SDL_GetBasePath();
  return PathDataView;
}

bool PathIsDirectory(const char *path) {
  SDL_PathInfo pi;
  const auto ret = SDL_GetPathInfo(path, &pi);
  return (ret && (pi.type == SDL_PATHTYPE_DIRECTORY));
}
