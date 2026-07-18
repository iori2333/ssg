///
/// Data initialization entry point
///
#include "init.h"

#include <SDL3/SDL_messagebox.h>

#include "pack_manager.h"

std::optional<std::string> DataInit() {
  if (!packs.LoadAll()) {
    auto missing = packs.MissingFilesReport();
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing game data files",
                             missing.c_str(), nullptr);
    return missing;
  }
  return std::nullopt;
}

void DataCleanup() {}
