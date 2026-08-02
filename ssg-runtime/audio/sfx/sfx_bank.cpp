#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>

#include <SDL3/SDL_audio.h>
#include <miniaudio.h>

#include "sfx_bank.h"

#include "audio/core/audio_types.h"
#include "audio/sfx.h"

namespace audio::sfx {
namespace {

ma_format HelpFormatFrom(SDL_AudioFormat format) {
  switch (format) {
  case SDL_AUDIO_U8:
    return ma_format_u8;
  case SDL_AUDIO_S16:
    return ma_format_s16;
  case SDL_AUDIO_S32:
    return ma_format_s32;
  case SDL_AUDIO_F32:
    return ma_format_f32;
  default:
    return ma_format_unknown;
  }
}

} // namespace

SfxBank::Instance::~Instance() {
  ma_sound_uninit(&sound);
  ma_audio_buffer_ref_uninit(&data_source);
}

void SfxBank::Effect::Clear() {
  max = 0;
  now = 0;
  instances.clear();
  resampled_buffer.clear();
}

bool SfxBank::Effect::Loaded() const { return !instances.empty(); }

SfxBank::SfxBank(ma_engine &engine) : engine_(engine) {}

SfxBank::~SfxBank() { Shutdown(); }

AudioResult SfxBank::Initialize() {
  if (group_initialized_) {
    return AudioResult::Ok();
  }
  if (ma_sound_group_init(&engine_, MA_SOUND_FLAG_NO_PITCH, nullptr, &group_) !=
      MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize sound effect group");
  }
  group_initialized_ = true;
  return AudioResult::Ok();
}

void SfxBank::Shutdown() {
  for (auto &effect : effects_) {
    effect.Clear();
  }
  if (group_initialized_) {
    ma_sound_group_uninit(&group_);
    group_initialized_ = false;
  }
}

AudioResult SfxBank::Load(SfxId id, const SDL_AudioSpec &spec,
                          std::span<const std::uint8_t> pcm,
                          int max_instances) {
  const auto effect_index = std::to_underlying(id);
  if (effect_index >= kSfxObjectCount) {
    return AudioResult::Fail(AudioError::InvalidArgument,
                             "Sound effect id out of range");
  }
  if (max_instances == 0) {
    return AudioResult::Fail(AudioError::InvalidArgument,
                             "Sound effect requires at least one instance");
  }

  auto &effect = effects_[effect_index];
  effect.Clear();
  effect.instances.resize(max_instances);
  effect.max = max_instances;
  effect.now = 0;

  const auto *device = ma_engine_get_device(&engine_);
  const auto config = ma_data_converter_config_init(
      HelpFormatFrom(spec.format), device->playback.format, spec.channels,
      spec.channels, spec.freq, device->sampleRate);
  ma_data_converter converter{};
  if (ma_data_converter_init(&config, nullptr, &converter) != MA_SUCCESS) {
    effect.Clear();
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize SFX resampler");
  }

  const std::size_t input_frame_size =
      static_cast<std::size_t>(SDL_AUDIO_BYTESIZE(spec.format)) *
      static_cast<std::size_t>(spec.channels);
  const std::size_t output_frame_size =
      ma_get_bytes_per_frame(config.formatOut, config.channelsOut);
  ma_uint64 input_frames = (pcm.size() / input_frame_size);
  ma_uint64 output_frames = 0;
  if (ma_data_converter_get_expected_output_frame_count(
          &converter, input_frames, &output_frames) != MA_SUCCESS) {
    ma_data_converter_uninit(&converter, nullptr);
    effect.Clear();
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to calculate SFX output size");
  }

  effect.resampled_buffer.resize(output_frame_size * output_frames);
  if (ma_data_converter_process_pcm_frames(
          &converter, pcm.data(), &input_frames,
          effect.resampled_buffer.data(),
          &output_frames) != MA_SUCCESS) {
    ma_data_converter_uninit(&converter, nullptr);
    effect.Clear();
    return AudioResult::Fail(AudioError::DecodeFailed,
                             "Failed to resample sound effect");
  }
  ma_data_converter_uninit(&converter, nullptr);

  for (int i = 0; i < max_instances; i++) {
    if (!effect.instances[i]) {
      effect.instances[i] = std::make_unique<Instance>();
    }
    auto &instance = *effect.instances[i];
    if (ma_audio_buffer_ref_init(config.formatOut, config.channelsOut,
                                 effect.resampled_buffer.data(), output_frames,
                                 &instance.data_source) != MA_SUCCESS) {
      effect.Clear();
      return AudioResult::Fail(AudioError::BackendFailed,
                               "Failed to initialize SFX buffer");
    }
    if (ma_sound_init_from_data_source(
            &engine_, &instance.data_source,
            (MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION), &group_,
            &instance.sound) != MA_SUCCESS) {
      effect.Clear();
      return AudioResult::Fail(AudioError::BackendFailed,
                               "Failed to initialize SFX sound");
    }
  }
  return AudioResult::Ok();
}

void SfxBank::Play(SfxId id, float pan, bool loop) {
  const auto effect_index = std::to_underlying(id);
  if (effect_index >= kSfxObjectCount || !effects_[effect_index].Loaded()) {
    return;
  }
  auto &effect = effects_[effect_index];
  auto &instance = *effect.instances[effect.now];
  ma_sound_stop(&instance.sound);
  ma_sound_set_looping(&instance.sound, static_cast<ma_bool32>(loop));
  ma_sound_set_pan(&instance.sound, std::clamp(pan, -1.0F, 1.0F));
  ma_sound_seek_to_pcm_frame(&instance.sound, 0);
  ma_sound_start(&instance.sound);
  effect.now = ((effect.now + 1) % effect.max);
}

void SfxBank::Stop(SfxId id) {
  const auto effect_index = std::to_underlying(id);
  if (effect_index >= kSfxObjectCount || !effects_[effect_index].Loaded()) {
    return;
  }
  for (int i = 0; i < effects_[effect_index].max; i++) {
    ma_sound_stop(&effects_[effect_index].instances[i]->sound);
  }
}

void SfxBank::StopAll() {
  for (std::size_t id = 0; id < kSfxObjectCount; id++) {
    Stop(static_cast<SfxId>(id));
  }
}

void SfxBank::SetVolume(float linear) {
  if (group_initialized_) {
    ma_sound_group_set_volume(&group_, linear);
  }
}

bool SfxBank::IsLoaded(SfxId id) const {
  const auto effect_index = std::to_underlying(id);
  return effect_index < kSfxObjectCount && effects_[effect_index].Loaded();
}

} // namespace audio::sfx
