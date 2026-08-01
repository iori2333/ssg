#include "volume_ramp.h"

namespace audio {

void VolumeRamp::Set(float linear) noexcept {
  current_.store(linear, std::memory_order_relaxed);
  end_.store(linear, std::memory_order_relaxed);
  step_.store(0.0f, std::memory_order_relaxed);
  frames_remaining_.store(0, std::memory_order_relaxed);
}

void VolumeRamp::StartFade(float from, float to, uint64_t frames) noexcept {
  current_.store(from, std::memory_order_relaxed);
  end_.store(to, std::memory_order_relaxed);
  if (frames == 0) {
    step_.store(0.0f, std::memory_order_relaxed);
    frames_remaining_.store(0, std::memory_order_relaxed);
    current_.store(to, std::memory_order_relaxed);
    return;
  }
  step_.store((to - from) / static_cast<float>(frames),
              std::memory_order_relaxed);
  frames_remaining_.store(frames, std::memory_order_relaxed);
}

float VolumeRamp::Current() const noexcept {
  return current_.load(std::memory_order_relaxed);
}

float VolumeRamp::NextFrame() noexcept {
  const auto remaining = frames_remaining_.load(std::memory_order_relaxed);
  if (remaining == 0) {
    return current_.load(std::memory_order_relaxed);
  }

  float value = current_.load(std::memory_order_relaxed);
  if (remaining > 1) {
    value += step_.load(std::memory_order_relaxed);
  } else {
    value = end_.load(std::memory_order_relaxed);
  }
  current_.store(value, std::memory_order_relaxed);
  frames_remaining_.store(remaining - 1, std::memory_order_relaxed);
  return value;
}

} // namespace audio

