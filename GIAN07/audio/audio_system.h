/// Application-level audio lifecycle and configuration.

#pragma once

#include <string_view>

#include "audio/volume.h"

struct AudioConfig;
class MusicPlayer;

namespace data {
class SfxLoader;
}

class AudioSystem {
public:
  AudioSystem(MusicPlayer &music, data::SfxLoader &sound_effects)
      : music_(music), sound_effects_(sound_effects) {}
  ~AudioSystem();
  AudioSystem(const AudioSystem &) = delete;
  AudioSystem(AudioSystem &&) = delete;
  AudioSystem &operator=(const AudioSystem &) = delete;
  AudioSystem &operator=(AudioSystem &&) = delete;

  [[nodiscard]] bool Initialize(const AudioConfig &config);
  void Shutdown();

  [[nodiscard]] bool EnableBgm(bool enabled,
                               std::string_view soundfont = {});
  [[nodiscard]] bool EnableSfx(bool enabled);
  void SetVolumes(VOLUME bgm, VOLUME sfx);
  void SetNormalization(bool enabled);

private:
  MusicPlayer &music_;
  data::SfxLoader &sound_effects_;
  bool initialized_ = false;
};
