/// Miniaudio-backed sound effect bank.

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include <miniaudio.h>

#include "audio/core/audio_types.h"

struct SDL_AudioSpec;

namespace audio::sfx {

inline constexpr std::uint8_t kSfxObjectCount = 30;

class SfxBank {
public:
  explicit SfxBank(ma_engine &engine);
  ~SfxBank();
  SfxBank(const SfxBank &) = delete;
  SfxBank &operator=(const SfxBank &) = delete;
  SfxBank(SfxBank &&) = delete;
  SfxBank &operator=(SfxBank &&) = delete;

  AudioResult Initialize();
  void Shutdown();

  AudioResult Load(std::uint8_t id, const SDL_AudioSpec &spec,
                   std::span<const std::uint8_t> pcm,
                   std::uint8_t max_instances);
  void Play(std::uint8_t id, float pan = 0.0F, bool loop = false);
  void Stop(std::uint8_t id);
  void StopAll();
  void SetVolume(float linear);

  [[nodiscard]] bool IsLoaded(std::uint8_t id) const;

private:
  struct Instance {
    ma_audio_buffer_ref data_source{};
    ma_sound sound{};

    Instance() = default;
    ~Instance();
    Instance(const Instance &) = delete;
    Instance &operator=(const Instance &) = delete;
    Instance(Instance &&) = delete;
    Instance &operator=(Instance &&) = delete;
  };

  struct Effect {
    std::vector<std::byte> resampled_buffer;
    std::vector<std::unique_ptr<Instance>> instances;
    std::uint32_t max = 0;
    std::uint32_t now = 0;

    void Clear();
    [[nodiscard]] bool Loaded() const;
  };

  ma_engine &engine_;
  ma_sound_group group_{};
  bool group_initialized_ = false;
  std::array<Effect, kSfxObjectCount> effects_{};
};

} // namespace audio::sfx
