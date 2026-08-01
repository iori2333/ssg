/// Application-owned miniaudio engine wrapper.

#pragma once

#include <miniaudio.h>

#include "audio/core/audio_types.h"

namespace audio::backend {

class AudioEngine {
public:
  AudioEngine() = default;
  ~AudioEngine();
  AudioEngine(const AudioEngine &) = delete;
  AudioEngine &operator=(const AudioEngine &) = delete;

  AudioResult Initialize();
  void Shutdown();

  [[nodiscard]] ma_engine &Get();
  [[nodiscard]] bool IsInitialized() const;

private:
  ma_engine engine_{};
  bool initialized_ = false;
};

} // namespace audio::backend

