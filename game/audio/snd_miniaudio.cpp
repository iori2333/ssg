///
/// Sound via miniaudio
///

#include <algorithm>
#include <ranges>

#include <SDL3/SDL_audio.h>
#include <miniaudio.h>

#include "bgm.h"
#include "bgm_track.h"
#include "snd_backend.h"

#include "util/guard.h"

// Helpers
// -------

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
// -------

struct BgmObject {
  ma_data_source_base data_source{};
  ma_sound sound{};
  std::shared_ptr<bgm::Track> track = nullptr;

  bool Clear() {
    if (track) {
      ma_sound_uninit(&sound);
      ma_data_source_uninit(&data_source);
      track = nullptr;
    }
    return false;
  }
};

static_assert(
    (offsetof(BgmObject, data_source) == 0),
    "The `ma_data_source_base` object must come first in the structure");

static ma_result BgmGetDataFormat(ma_data_source *pDataSource,
                                  ma_format *pFormat, ma_uint32 *pChannels,
                                  ma_uint32 *pSampleRate,
                                  ma_channel *pChannelMap,
                                  size_t channelMapCap) {
  const auto *bgm = static_cast<BgmObject *>(pDataSource);
  if (!bgm->track) {
    return MA_UNAVAILABLE;
  }
  switch (bgm->track->pcmf.format) {
  case PcmSampleFormat::Int16:
    *pFormat = ma_format_s16;
    break;
  case PcmSampleFormat::Int32:
    *pFormat = ma_format_s32;
    break;
  }
  *pChannels = bgm->track->pcmf.channels;
  *pSampleRate = bgm->track->pcmf.samplingrate;
  return MA_SUCCESS;
}

static ma_result BgmRead(ma_data_source *pDataSource, void *pFramesOut,
                         ma_uint64 frameCount, ma_uint64 *pFramesRead) {
  const auto *bgm = static_cast<BgmObject *>(pDataSource);
  if (!bgm->track) {
    return MA_UNAVAILABLE;
  }
  const auto sample_size = bgm->track->pcmf.SampleSize();

  if ((frameCount * sample_size) > (std::numeric_limits<size_t>::max)()) {
    return MA_TOO_BIG;
  }
  const size_t buf_size = (frameCount * sample_size);
  if (!bgm->track->Decode({static_cast<std::byte *>(pFramesOut), buf_size})) {
    return MA_INVALID_DATA;
  }
  *pFramesRead = frameCount;
  return MA_SUCCESS;
}

static const ma_data_source_vtable kBgmVtable = {
    .onRead = BgmRead,
    .onGetDataFormat = BgmGetDataFormat,
};

struct ResampledInstance {
  ma_audio_buffer_ref data_source{};
  ma_sound sound{};

  ~ResampledInstance() {
    ma_sound_uninit(&sound);
    ma_audio_buffer_ref_uninit(&data_source);
  }
};

struct SoundEffect {
  // Backing storage for the individual `ma_audio_buffer` instances.
  std::unique_ptr<std::byte[]> resampled_buffer = nullptr;

  std::unique_ptr<ResampledInstance[]> instance;
  unsigned int max = 0;
  unsigned int now = 0;

  bool Clear() {
    max = 0;
    now = 0;
    instance = nullptr;
    resampled_buffer = nullptr;
    return false;
  }

  bool Loaded() { return (instance.get() != nullptr); }
};

namespace {

struct AudioBackendState {
  ma_engine engine;
  ma_sound_group sound_effect_group;
  BgmObject bgm;
  SoundEffect sound_effects[kSoundObjectCount];
};

AudioBackendState &State() {
  static AudioBackendState state;
  return state;
}

} // namespace

bool AudioBackendInitialize() {
  return (ma_engine_init(nullptr, &State().engine) == MA_SUCCESS);
}

void AudioBackendCleanup() { ma_engine_uninit(&State().engine); }

bool AudioBackendInitializeBgm() { return true; }

void AudioBackendCleanupBgm() { State().bgm.Clear(); }

bool AudioBackendLoadBgm(std::shared_ptr<bgm::Track> track) {
  ma_result result = MA_SUCCESS;

  State().bgm.Clear();
  State().bgm.track = track;

  auto config = ma_data_source_config_init();
  config.vtable = &kBgmVtable;

  result = ma_data_source_init(&config, &State().bgm.data_source);
  if (result != MA_SUCCESS) {
    return result;
  }
  result = ma_sound_init_from_data_source(
      &State().engine, &State().bgm.data_source,
      MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, &State().bgm.sound);
  if (result != MA_SUCCESS) {
    return State().bgm.Clear();
  }
  return true;
}

void AudioBackendPlayBgm() {
  if (!State().bgm.track) {
    return;
  }
  ma_sound_start(&State().bgm.sound);
}

void AudioBackendStopBgm() {
  if (!State().bgm.track) {
    return;
  }
  ma_sound_stop(&State().bgm.sound);
}

std::chrono::milliseconds AudioBackendBgmPlayTime() {
  if (!State().bgm.track) {
    return std::chrono::milliseconds::zero();
  }
  const auto ret = ma_sound_get_time_in_milliseconds(&State().bgm.sound);
  return std::chrono::milliseconds{ret};
}

void AudioBackendUpdateBgmVolume() {
  if (!State().bgm.track) {
    return;
  }
  ma_sound_set_volume(&State().bgm.sound,
                      (BgmGainFactor() * LinearVolumeFactor(AudioBgmVolume())));
}

void AudioBackendUpdateBgmTempo() {
  if (!State().bgm.track) {
    return;
  }
  const auto t = BgmTempoFactor();
  ma_sound_set_pitch(&State().bgm.sound, t);
}

bool AudioBackendInitializeSoundEffects() {
  const auto ret = ma_sound_group_init(&State().engine, MA_SOUND_FLAG_NO_PITCH,
                                       nullptr, &State().sound_effect_group);
  return (ret == MA_SUCCESS);
}

void AudioBackendCleanupSoundEffects() {
  for (auto &se : State().sound_effects) {
    se.Clear();
  }
  ma_sound_group_uninit(&State().sound_effect_group);
}

void AudioBackendUpdateSoundEffectVolume() {
  ma_sound_group_set_volume(&State().sound_effect_group,
                            LinearVolumeFactor(AudioSoundEffectVolume()));
}

bool AudioBackendLoadSoundEffect(uint8_t id, SoundInstanceId max,
                                 const SDL_AudioSpec &spec,
                                 std::span<const uint8_t> pcm) {
  if (id >= kSoundObjectCount) {
    return false;
  }
  auto &se = State().sound_effects[id];

  ma_result result = MA_SUCCESS;
  se.Clear();

  if (max == 0) {
    return false;
  }

  se.instance = std::unique_ptr<ResampledInstance[]>(
      new (std::nothrow) ResampledInstance[max]);
  if (!se.instance) {
    return false;
  }
  se.max = max;
  se.now = 0;

  // Resample to the native sample rate and create a single authoritative
  // buffer. We still keep the original channel count, though, since mono
  // expansion is SSE2-optimized.
  const auto *device = ma_engine_get_device(&State().engine);
  const auto config = ma_data_converter_config_init(
      HelpFormatFrom(spec.format), device->playback.format, spec.channels,
      spec.channels, spec.freq, device->sampleRate);
  ma_data_converter converter;
  result = ma_data_converter_init(&config, NULL, &converter);
  if (result != MA_SUCCESS) {
    return se.Clear();
  }
  auto conv_guard = util::MakeGuard(
      &converter, [](auto *c) { ma_data_converter_uninit(c, nullptr); });
  const size_t input_frame_size = SDL_AUDIO_FRAMESIZE(spec);
  const size_t output_frame_size =
      ma_get_bytes_per_frame(config.formatOut, config.channelsOut);
  ma_uint64 input_frames = (pcm.size() / input_frame_size);
  ma_uint64 output_frames = 0;
  result = ma_data_converter_get_expected_output_frame_count(
      &converter, input_frames, &output_frames);
  if (result != MA_SUCCESS) {
    return se.Clear();
  }
  se.resampled_buffer = std::unique_ptr<std::byte[]>(
      new (std::nothrow) std::byte[output_frame_size * output_frames]);
  if (!se.resampled_buffer) {
    return false;
  }
  result = ma_data_converter_process_pcm_frames(
      &converter, pcm.data(), &input_frames, se.resampled_buffer.get(),
      &output_frames);
  if (result != MA_SUCCESS) {
    return se.Clear();
  }

  for (const auto i : std::views::iota(0u, se.max)) {
    auto &instance = se.instance[i];
    result = ma_audio_buffer_ref_init(config.formatOut, config.channelsOut,
                                      se.resampled_buffer.get(), output_frames,
                                      &instance.data_source);
    if (result != MA_SUCCESS) {
      return se.Clear();
    }
    result = ma_sound_init_from_data_source(
        &State().engine, &instance.data_source,
        (MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION),
        &State().sound_effect_group, &instance.sound);
    if (result != MA_SUCCESS) {
      return se.Clear();
    }
  }
  return true;
}

void AudioBackendPlaySoundEffect(uint8_t id, float pan, bool loop) {
  if (id >= kSoundObjectCount) {
    return;
  }
  auto &se = State().sound_effects[id];
  if (!se.Loaded()) {
    return;
  }
  auto &instance = se.instance[se.now];

  // I *guess* we don't have to worry about thread safety here? These
  // operations are either atomic (loop, seek target, playback state), or
  // passed by value (pan). The worst thing that can happen is that these
  // changes only apply to the next iteration of the audio thread. Which is
  // probably still better than busy-waiting for the [isReading] lock of the
  // engine's node graph.
  ma_sound_stop(&instance.sound); // Both are necessary!
  ma_sound_set_looping(&instance.sound, loop);
  ma_sound_set_pan(&instance.sound, std::clamp(pan, -1.0f, 1.0f));
  ma_sound_seek_to_pcm_frame(&instance.sound, 0); // Both are necessary!
  ma_sound_start(&instance.sound);

  se.now = ((se.now + 1) % se.max);
}

void AudioBackendStopSoundEffect(uint8_t id) {
  if (id >= kSoundObjectCount) {
    return;
  }
  auto &se = State().sound_effects[id];
  for (const auto i : std::views::iota(0u, se.max)) {
    auto &instance = se.instance[i];
    ma_sound_stop(&instance.sound);
  }
}

void AudioBackendPauseAll() { ma_engine_stop(&State().engine); }

void AudioBackendResumeAll() { ma_engine_start(&State().engine); }
