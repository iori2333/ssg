/// Title screen scene.

#pragma once

#include <cstdint>

#include "gfx/coords.h"
#include "gfx/text.h"
#include "sys/input.h"

class GameSession;
class MusicPlayer;
class UIManager;
struct ConfigData;

namespace data {
class GraphicsLoader;
class SfxLoader;
} // namespace data

enum class TitleSceneResult : uint8_t {
  Running,
  QuitRequested,
  StartGame,
  StartExtra,
  StartDemo,
  OpenReplay,
  OpenScore,
  OpenMusicRoom,
  OpenBulletGallery,
};

class TitleScene {
public:
  TitleScene(ConfigData &config, data::GraphicsLoader &graphics,
             data::SfxLoader &sound_effects, MusicPlayer &music,
             GameSession &session, UIManager &ui)
      : config_(config), graphics_(graphics), sound_effects_(sound_effects),
        music_(music), session_(session), ui_(ui) {}

  [[nodiscard]] bool Enter(INPUT_BITS initial_input, bool change_music);
  [[nodiscard]] TitleSceneResult Update(INPUT_BITS input, bool should_draw);

private:
  void InitVersion();
  void DrawVersion(PIXEL_COORD top) const;

  ConfigData &config_;
  data::GraphicsLoader &graphics_;
  data::SfxLoader &sound_effects_;
  MusicPlayer &music_;
  GameSession &session_;
  UIManager &ui_;
  TEXTRENDER_RECT_ID version_rect_{};
  WINDOW_COORD version_left_ = 0;
  uint16_t demo_timer_ = 0;
};
