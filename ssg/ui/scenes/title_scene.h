/// Title screen scene.

#pragma once

#include <cstdint>

#include "gfx/core/coords.h"
#include "gfx/text/text.h"
#include "sys/input.h"

class GameSession;
class MusicPlayer;
class UiManager;

namespace audio {
class AudioSystem;
}

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
             GameSession &session, UiManager &ui, audio::AudioSystem &audio)
      : graphics_(graphics), music_(music), session_(session), ui_(ui),
        audio_(audio) {}

  [[nodiscard]] bool Enter(InputBits initial_input, bool change_music);
  [[nodiscard]] TitleSceneResult Update(InputBits input, bool should_draw);

private:
  void InitVersion();
  void DrawVersion(int top) const;

  data::GraphicsLoader &graphics_;
  MusicPlayer &music_;
  GameSession &session_;
  UiManager &ui_;
  audio::AudioSystem &audio_;
  TextRenderRectId version_rect_{};
  int version_left_ = 0;
  int demo_timer_ = 0;
};
