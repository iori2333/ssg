///
/// Common paths and path manipulation via SDL
///

// SDL headers must come first to avoid import→#include bugs on Clang 19.
#include <SDL3/SDL_filesystem.h>

#include "constants.h"
#include "game/path.h"

#ifndef WIN32
constexpr auto SDL_free_deleter = [](auto *p) { SDL_free(p); };
static std::unique_ptr<char[], decltype(SDL_free_deleter)> PathData = {nullptr};
#endif
static std::string_view PathDataView;

std::string_view PathForData(void) {
  if (PathDataView.data() != nullptr) {
    return PathDataView;
  }
#ifdef WIN32
  PathDataView = SDL_GetBasePath();
#else
#ifdef PATH_XDG_DATA_HOME_HAS_APP_ID
  auto *path_base = SDL_GetPrefPath(nullptr, "");
#else
  auto *path_base = SDL_GetPrefPath(GAME_ORG, GAME_APP);
#endif
  PathData.reset(path_base);
  PathDataView = PathData.get();

  // Remove the unsightly second slash in case we passed an empty app
  if (PathDataView.ends_with("//")) {
    PathDataView.remove_suffix(1);
  }
#endif

  return PathDataView;
}

bool PathIsDirectory(const char *path) {
  SDL_PathInfo pi;
  const auto ret = SDL_GetPathInfo(path, &pi);
  return (ret && (pi.type == SDL_PATHTYPE_DIRECTORY));
}
