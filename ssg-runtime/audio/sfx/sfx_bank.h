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
#include "audio/sfx.h"

struct SDL_AudioSpec;

namespace audio::sfx {

inline constexpr std::size_t kSfxObjectCount = 30;

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

  AudioResult Load(SfxId id, const SDL_AudioSpec &spec,
                   std::span<const std::uint8_t> pcm,
                   int max_instances);
  void Play(SfxId id, float pan = 0.0F, bool loop = false);
  void Stop(SfxId id);
  void StopAll();
  void SetVolume(float linear);

  [[nodiscard]] bool IsLoaded(SfxId id) const;

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
    std::vector<uint8_t> resampled_buffer;
    std::vector<std::unique_ptr<Instance>> instances;
    int max = 0;
    int now = 0;

    void Clear();
    [[nodiscard]] bool Loaded() const;
  };

  ma_engine &engine_;
  ma_sound_group group_{};
  bool group_initialized_ = false;
  std::array<Effect, kSfxObjectCount> effects_{};
};

} // namespace audio::sfx
