///
/// Data initialization entry point
///
#include "init.h"

#include <SDL3/SDL_messagebox.h>

#include "gameflow/game_main.h"
#include "gameflow/gameflow_manager.h"
#include "pack_manager.h"

void DataInit() {
  if (!packs.LoadAll()) {
    auto missing = packs.MissingFilesReport();
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Missing game data files",
                             missing.c_str(), nullptr);
    GameFlow.game_main = [](bool &quit) { quit = true; };
    GameFlow.current_state = GameState::External;
    return;
  }
  SProjectInit();
}

void DataCleanup() {}
