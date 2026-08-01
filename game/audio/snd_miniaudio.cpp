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

struct RESAMPLED_INSTANCE {
  ma_audio_buffer_ref data_source{};
  ma_sound sound{};

  ~RESAMPLED_INSTANCE() {
    ma_sound_uninit(&sound);
    ma_audio_buffer_ref_uninit(&data_source);
  }
};

struct SE {
  // Backing storage for the individual `ma_audio_buffer` instances.
  std::unique_ptr<std::byte[]> resampled_buffer = nullptr;

  std::unique_ptr<RESAMPLED_INSTANCE[]> instance;
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

static ma_engine Engine;
static ma_sound_group SEGroup;
static BgmObject BgmObjectState;
static SE SndObj[kSoundObjectCount];

bool AudioBackendInitialize(void) {
  return (ma_engine_init(nullptr, &Engine) == MA_SUCCESS);
}

void AudioBackendCleanup(void) { ma_engine_uninit(&Engine); }

bool AudioBackendInitializeBgm(void) { return true; }

void AudioBackendCleanupBgm(void) { BgmObjectState.Clear(); }

bool AudioBackendLoadBgm(std::shared_ptr<bgm::Track> track) {
  ma_result result = MA_SUCCESS;

  BgmObjectState.Clear();
  BgmObjectState.track = track;

  auto config = ma_data_source_config_init();
  config.vtable = &kBgmVtable;

  result = ma_data_source_init(&config, &BgmObjectState.data_source);
  if (result != MA_SUCCESS) {
    return result;
  }
  result = ma_sound_init_from_data_source(&Engine, &BgmObjectState.data_source,
                                          MA_SOUND_FLAG_NO_SPATIALIZATION,
                                          nullptr, &BgmObjectState.sound);
  if (result != MA_SUCCESS) {
    return BgmObjectState.Clear();
  }
  return true;
}

void AudioBackendPlayBgm(void) {
  if (!BgmObjectState.track) {
    return;
  }
  ma_sound_start(&BgmObjectState.sound);
}

void AudioBackendStopBgm(void) {
  if (!BgmObjectState.track) {
    return;
  }
  ma_sound_stop(&BgmObjectState.sound);
}

std::chrono::milliseconds AudioBackendBgmPlayTime(void) {
  if (!BgmObjectState.track) {
    return std::chrono::milliseconds::zero();
  }
  const auto ret = ma_sound_get_time_in_milliseconds(&BgmObjectState.sound);
  return std::chrono::milliseconds{ret};
}

void AudioBackendUpdateBgmVolume(void) {
  if (!BgmObjectState.track) {
    return;
  }
  ma_sound_set_volume(&BgmObjectState.sound,
                      (BgmGainFactor() * LinearVolumeFactor(AudioBgmVolume())));
}

void AudioBackendUpdateBgmTempo(void) {
  if (!BgmObjectState.track) {
    return;
  }
  const auto t = BgmTempoFactor();
  ma_sound_set_pitch(&BgmObjectState.sound, t);
}

bool AudioBackendInitializeSoundEffects(void) {
  const auto ret =
      ma_sound_group_init(&Engine, MA_SOUND_FLAG_NO_PITCH, nullptr, &SEGroup);
  return (ret == MA_SUCCESS);
}

void AudioBackendCleanupSoundEffects(void) {
  for (auto &se : SndObj) {
    se.Clear();
  }
  ma_sound_group_uninit(&SEGroup);
}

void AudioBackendUpdateSoundEffectVolume(void) {
  ma_sound_group_set_volume(&SEGroup,
                            LinearVolumeFactor(AudioSoundEffectVolume()));
}

bool AudioBackendLoadSoundEffect(uint8_t id, SoundInstanceId max,
                                 const SDL_AudioSpec &spec,
                                 std::span<const uint8_t> pcm) {
  if (id >= kSoundObjectCount) {
    return false;
  }
  auto &se = SndObj[id];

  ma_result result = MA_SUCCESS;
  se.Clear();

  if (max == 0) {
    return false;
  }

  se.instance = std::unique_ptr<RESAMPLED_INSTANCE[]>(
      new (std::nothrow) RESAMPLED_INSTANCE[max]);
  if (!se.instance) {
    return false;
  }
  se.max = max;
  se.now = 0;

  // Resample to the native sample rate and create a single authoritative
  // buffer. We still keep the original channel count, though, since mono
  // expansion is SSE2-optimized.
  const auto *device = ma_engine_get_device(&Engine);
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
        &Engine, &instance.data_source,
        (MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION), &SEGroup,
        &instance.sound);
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
  auto &se = SndObj[id];
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
  auto &se = SndObj[id];
  for (const auto i : std::views::iota(0u, se.max)) {
    auto &instance = se.instance[i];
    ma_sound_stop(&instance.sound);
  }
}

void AudioBackendPauseAll(void) { ma_engine_stop(&Engine); }

void AudioBackendResumeAll(void) { ma_engine_start(&Engine); }
