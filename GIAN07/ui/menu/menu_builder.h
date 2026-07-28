///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#pragma once

#include <functional>
#include <memory>

#include "menu_tree.h"

struct ConfigData;
class DisplayController;
class MusicPlayer;

namespace data {
class GraphicsLoader;
class SfxLoader;
} // namespace data

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
  data::SfxLoader &sound_effects;
  MusicPlayer &music;
};

std::unique_ptr<IMenuNode>
BuildMainMenuTree(ConfigData &cfg, MainMenuServices services,
                  std::function<void(MainMenuAction)> on_action);

} // namespace menu
