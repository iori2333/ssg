/// Title screen scene.

#pragma once

#include <cstdint>

#include "gfx/coords.h"
#include "gfx/text.h"
#include "sys/input.h"

class GameSession;
class MusicPlayer;
class UIManager;

namespace data {
class GraphicsLoader;
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
  TitleScene(data::GraphicsLoader &graphics, MusicPlayer &music,
             GameSession &session, UIManager &ui)
      : graphics_(graphics), music_(music), session_(session), ui_(ui) {}

  [[nodiscard]] bool Enter(INPUT_BITS initial_input, bool change_music);
  [[nodiscard]] TitleSceneResult Update(INPUT_BITS input, bool should_draw);

private:
  void InitVersion();
  void DrawVersion(PIXEL_COORD top) const;

  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  GameSession &session_;
  UIManager &ui_;
  TEXTRENDER_RECT_ID version_rect_{};
  WINDOW_COORD version_left_ = 0;
  uint16_t demo_timer_ = 0;
};
