/// BGM waveform playback backed by miniaudio.

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <string_view>

#include <miniaudio.h>

#include "audio/core/audio_types.h"

namespace audio::bgm {

class WaveformSource;

class WaveformPlayback {
public:
  explicit WaveformPlayback(ma_engine &engine);
  ~WaveformPlayback();
  WaveformPlayback(const WaveformPlayback &) = delete;
  WaveformPlayback &operator=(const WaveformPlayback &) = delete;

  AudioResult Load(std::string_view path);
  void Unload();

  void Play();
  void Stop();
  void Pause();
  void Resume();
  void SetVolume(float linear);
  void SetPitch(float factor);
  void FadeOut(float from, std::chrono::milliseconds duration);

  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] std::string_view Title() const;
  [[nodiscard]] std::chrono::milliseconds PlayTime() const;
  [[nodiscard]] float FadeVolumeLinear() const;
  [[nodiscard]] std::optional<float> GainFactor() const;

private:
  ma_engine &engine_;
  std::unique_ptr<WaveformSource> source_;
  ma_sound sound_{};
  bool sound_initialized_ = false;
};

} // namespace audio::bgm
