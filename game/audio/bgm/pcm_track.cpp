#include "pcm_track.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <miniaudio.h>

#include "pcm_source.h"

#include "audio/core/audio_types.h"

namespace audio::bgm {
namespace {

float LinearVolume(Volume volume) {
  return (static_cast<float>(volume) / static_cast<float>(kMaxVolume));
}

constexpr std::uint8_t kTempoDenominator = 128;

} // namespace

class PcmTrack::Source {
public:
  Source() = default;
  ~Source();
  Source(const Source &) = delete;
  Source &operator=(const Source &) = delete;

  AudioResult Load(std::string_view path);
  void Unload();

  void FadeOut(float from, std::chrono::milliseconds duration);
  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] std::string_view Title() const;
  [[nodiscard]] float FadeVolumeLinear() const;
  [[nodiscard]] std::optional<float> GainFactor() const;
  [[nodiscard]] ma_data_source *DataSource();

  static ma_result GetDataFormat(ma_data_source *data_source, ma_format *format,
                                 ma_uint32 *channels, ma_uint32 *sample_rate,
                                 ma_channel *channel_map,
                                 size_t channel_map_cap);
  static ma_result Read(ma_data_source *data_source, void *frames_out,
                        ma_uint64 frame_count, ma_uint64 *frames_read);

private:
  ma_data_source_base data_source_{};
  std::unique_ptr<bgm::PcmStream> source_;
  std::string title_;
  bool initialized_ = false;
};

namespace {

constexpr ma_data_source_vtable kWaveformVtable = {
    .onRead = &PcmTrack::Source::Read,
    .onGetDataFormat = &PcmTrack::Source::GetDataFormat,
};

} // namespace

PcmTrack::Source::~Source() { Unload(); }

AudioResult PcmTrack::Source::Load(std::string_view path) {
  Unload();
  source_ = OpenPcmStream(path);
  if (!source_) {
    return AudioResult::Fail(AudioError::TrackOpenFailed,
                             "Failed to open waveform track");
  }
  title_ = source_->metadata.title;

  ma_data_source_config config = ma_data_source_config_init();
  config.vtable = &kWaveformVtable;
  if (ma_data_source_init(&config, &data_source_) != MA_SUCCESS) {
    source_.reset();
    title_.clear();
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize waveform data source");
  }
  initialized_ = true;
  return AudioResult::Ok();
}

void PcmTrack::Source::Unload() {
  if (initialized_) {
    ma_data_source_uninit(&data_source_);
    initialized_ = false;
  }
  source_.reset();
  title_.clear();
}

void PcmTrack::Source::FadeOut(float from, std::chrono::milliseconds duration) {
  if (source_) {
    source_->FadeOut(from, duration);
  }
}

bool PcmTrack::Source::IsLoaded() const {
  return initialized_ && source_ != nullptr;
}

std::string_view PcmTrack::Source::Title() const { return title_; }

float PcmTrack::Source::FadeVolumeLinear() const {
  return source_ ? source_->FadeVolumeLinear() : 0.0f;
}

std::optional<float> PcmTrack::Source::GainFactor() const {
  if (!source_ || !source_->metadata.gain_factor) {
    return std::nullopt;
  }
  return source_->metadata.gain_factor;
}

ma_data_source *PcmTrack::Source::DataSource() {
  return reinterpret_cast<ma_data_source *>(&data_source_);
}

ma_result PcmTrack::Source::GetDataFormat(
    ma_data_source *data_source, ma_format *format, ma_uint32 *channels,
    ma_uint32 *sample_rate, ma_channel *channel_map, size_t channel_map_cap) {
  auto *self = reinterpret_cast<PcmTrack::Source *>(data_source);
  if (!self->source_) {
    return MA_UNAVAILABLE;
  }
  switch (self->source_->pcmf.format) {
  case PcmSampleFormat::Int16:
    *format = ma_format_s16;
    break;
  case PcmSampleFormat::Int32:
    *format = ma_format_s32;
    break;
  }
  *channels = self->source_->pcmf.channels;
  *sample_rate = self->source_->pcmf.samplingrate;
  (void)channel_map;
  (void)channel_map_cap;
  return MA_SUCCESS;
}

ma_result PcmTrack::Source::Read(ma_data_source *data_source, void *frames_out,
                                 ma_uint64 frame_count,
                                 ma_uint64 *frames_read) {
  auto *self = reinterpret_cast<PcmTrack::Source *>(data_source);
  if (!self->source_) {
    return MA_UNAVAILABLE;
  }
  const auto sample_size = self->source_->pcmf.SampleSize();
  if ((frame_count > (std::numeric_limits<std::size_t>::max() / sample_size))) {
    return MA_TOO_BIG;
  }
  const auto buffer_size = static_cast<std::size_t>(frame_count * sample_size);
  if (!self->source_->Decode(
          {static_cast<std::byte *>(frames_out), buffer_size})) {
    return MA_INVALID_DATA;
  }
  *frames_read = frame_count;
  return MA_SUCCESS;
}

PcmTrack::PcmTrack(ma_engine &engine) : engine_(engine) {}

PcmTrack::~PcmTrack() { Unload(); }

AudioResult PcmTrack::Load(std::string_view path) {
  Unload();

  auto source = std::make_unique<Source>();
  auto result = source->Load(path);
  if (!result.success) {
    return result;
  }

  if (ma_sound_init_from_data_source(&engine_, source->DataSource(),
                                     MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr,
                                     &sound_) != MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize waveform sound");
  }
  source_ = std::move(source);
  sound_initialized_ = true;
  return AudioResult::Ok();
}

void PcmTrack::Unload() {
  if (sound_initialized_) {
    ma_sound_uninit(&sound_);
    sound_initialized_ = false;
  }
  source_.reset();
  playing_.store(false);
}

void PcmTrack::Play() {
  if (IsLoaded()) {
    ma_sound_start(&sound_);
    playing_.store(true);
  }
}

void PcmTrack::Stop() {
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
  playing_.store(false);
}

void PcmTrack::Pause() {
  if (!playing_.exchange(false)) {
    return;
  }
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
}

void PcmTrack::Resume() {
  if (playing_.load() || !IsLoaded()) {
    return;
  }
  ma_sound_start(&sound_);
  playing_.store(true);
}

void PcmTrack::SetVolume(Volume volume) {
  volume_ = std::min(volume, kMaxVolume);
  ApplyVolume();
}

void PcmTrack::SetTempo(std::int8_t tempo) {
  tempo_ = std::clamp(tempo, std::int8_t{-100}, std::int8_t{100});
  if (!IsLoaded()) {
    return;
  }
  const auto numerator = static_cast<std::uint8_t>(kTempoDenominator + tempo_);
  ma_sound_set_pitch(&sound_, static_cast<float>(numerator) /
                                  static_cast<float>(kTempoDenominator));
}

void PcmTrack::SetGainApplied(bool enabled) {
  gain_applied_ = enabled;
  ApplyVolume();
}

void PcmTrack::FadeOut(float volume_start, std::chrono::milliseconds duration) {
  if (IsLoaded()) {
    source_->FadeOut(volume_start, duration);
  }
}

void PcmTrack::Tick(std::chrono::milliseconds) {
  if (!IsPlaying() || !IsLoaded()) {
    return;
  }
  if (FadeVolumeLinear() <= 0.0f) {
    Stop();
  }
}

bool PcmTrack::IsLoaded() const {
  return sound_initialized_ && source_ && source_->IsLoaded();
}

bool PcmTrack::IsPlaying() const { return playing_.load(); }

BgmMode PcmTrack::Mode() const { return BgmMode::Waveform; }

std::string_view PcmTrack::Title() const {
  return source_ ? source_->Title() : std::string_view{};
}

std::chrono::milliseconds PcmTrack::PlayTime() const {
  if (!IsLoaded()) {
    return std::chrono::milliseconds::zero();
  }
  return std::chrono::milliseconds{ma_sound_get_time_in_milliseconds(&sound_)};
}

float PcmTrack::FadeVolumeLinear() const {
  return source_ ? source_->FadeVolumeLinear() : 0.0f;
}

void PcmTrack::ApplyVolume() {
  if (!IsLoaded()) {
    return;
  }
  float gain = 1.0f;
  if (gain_applied_) {
    if (const auto gain_factor = source_->GainFactor(); gain_factor) {
      gain = *gain_factor;
    }
  }
  ma_sound_set_volume(&sound_, LinearVolume(volume_) * gain);
}

} // namespace audio::bgm
