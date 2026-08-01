/// BGM waveform playback around a miniaudio sound.

#pragma once

#include <chrono>
#include <memory>
#include <string_view>

#include <miniaudio.h>

#include "audio/core/audio_types.h"
#include "waveform_source.h"

namespace audio::stream {

class WaveformPlayer {
public:
  explicit WaveformPlayer(ma_engine &engine);
  ~WaveformPlayer();
  WaveformPlayer(const WaveformPlayer &) = delete;
  WaveformPlayer &operator=(const WaveformPlayer &) = delete;

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
  [[nodiscard]] ::bgm::Track &Track();

private:
  ma_engine &engine_;
  std::unique_ptr<WaveformSource> source_;
  ma_sound sound_{};
  bool sound_initialized_ = false;
};

} // namespace audio::stream
