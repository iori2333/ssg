/// Unified playback interface for BGM sources.

#pragma once

#include <chrono>
#include <cstdint>

#include "audio/core/audio_types.h"

namespace audio::bgm {

class Track {
public:
  Track() = default;
  Track(const Track &) = delete;
  Track &operator=(const Track &) = delete;
  Track(Track &&) = delete;
  Track &operator=(Track &&) = delete;
  virtual ~Track() = default;

  virtual void Play() = 0;
  virtual void Stop() = 0;
  virtual void Pause() = 0;
  virtual void Resume() = 0;
  virtual void FadeOut(float volume_start,
                       std::chrono::milliseconds duration) = 0;
  virtual void SetVolume(Volume volume) = 0;
  virtual void SetTempo(std::int8_t tempo) = 0;
  virtual void Tick(std::chrono::milliseconds delta) = 0;

  // Advances background state while this track is not the active output.
  virtual void TickBackground(std::chrono::milliseconds delta) {}

  [[nodiscard]] virtual bool IsLoaded() const = 0;
  [[nodiscard]] virtual bool IsPlaying() const = 0;
  [[nodiscard]] virtual BgmMode Mode() const = 0;
  [[nodiscard]] virtual std::chrono::milliseconds PlayTime() const = 0;
  [[nodiscard]] virtual float FadeVolumeLinear() const = 0;
};

} // namespace audio::bgm
