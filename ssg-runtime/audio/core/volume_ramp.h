/// Lock-free volume ramp intended for audio callback consumption.

#pragma once

#include <atomic>
#include <cstdint>

namespace audio {

class VolumeRamp {
public:
  void Set(float linear) noexcept;

  // Starts a fade over [frames] audio frames. The control thread calls this;
  // the audio callback only calls Current()/NextFrame().
  void StartFade(float from, float to, uint64_t frames) noexcept;

  [[nodiscard]] float Current() const noexcept;

  // Advances one audio frame and returns the new linear volume.
  [[nodiscard]] float NextFrame() noexcept;

private:
  std::atomic<float> current_{1.0F};
  std::atomic<float> step_{0.0F};
  std::atomic<float> end_{0.0F};
  std::atomic<uint64_t> frames_remaining_{0};
};

} // namespace audio
