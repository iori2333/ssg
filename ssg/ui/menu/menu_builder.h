///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#pragma once

#include <functional>
#include <memory>

#include "menu_tree.h"

struct ConfigData;
class DisplayController;
class InputSystem;
class MusicPlayer;

namespace audio {
class AudioSystem;
}

namespace data {
class SfxLoader;
}

namespace i18n {
class Localization;
}

namespace menu {

enum class MainMenuAction : uint8_t {
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
  audio::AudioSystem &audio;
  data::SfxLoader &sound_effects;
  MusicPlayer &music;
  i18n::Localization &localization;
};

std::unique_ptr<IMenuNode>
BuildMainMenuTree(ConfigData &cfg, MainMenuServices services,
                  const std::function<void(MainMenuAction)> &on_action);

} // namespace menu
