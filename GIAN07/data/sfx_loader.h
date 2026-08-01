///
/// SfxLoader - installs the game's sound-effect bank into the audio backend
///
#pragma once

#include "game_data.h"

namespace audio {
class AudioSystem;
}

namespace data {

class SfxLoader {
public:
  SfxLoader(const GameData &data, audio::AudioSystem &audio)
      : data_(&data), audio_(&audio) {}

  [[nodiscard]] bool Load() const;

private:
  const GameData *data_;
  audio::AudioSystem *audio_ = nullptr;
};

} // namespace data
