///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#pragma once

#include <functional>
#include <memory>

#include "menu_tree.h"

struct ConfigData;
class AudioSystem;
class DisplayController;
class InputSystem;
class MusicPlayer;

namespace i18n {
class Localization;
}

namespace menu {

enum class MainMenuAction {
  StartGame,
  StartExtra,
  OpenReplay,
  OpenScore,
  OpenMusicRoom,
  OpenBulletGallery,
};

struct MainMenuServices {
  DisplayController &display;
  InputSystem &input;
  AudioSystem &audio;
  MusicPlayer &music;
  i18n::Localization &localization;
};

std::unique_ptr<IMenuNode>
BuildMainMenuTree(ConfigData &cfg, MainMenuServices services,
                  std::function<void(MainMenuAction)> on_action);

} // namespace menu
