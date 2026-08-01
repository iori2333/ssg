#include "waveform_playback.h"

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

#include "audio/bgm/bgm_track.h"
#include "audio/core/audio_types.h"

namespace audio::bgm {

class WaveformSource {
public:
  WaveformSource() = default;
  ~WaveformSource();
  WaveformSource(const WaveformSource &) = delete;
  WaveformSource &operator=(const WaveformSource &) = delete;

  AudioResult Load(std::string_view path);
  void Unload();

  void FadeOut(float from, std::chrono::milliseconds duration);
  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] std::string_view Title() const;
  [[nodiscard]] float FadeVolumeLinear() const;
  [[nodiscard]] std::optional<float> GainFactor() const;
  [[nodiscard]] ma_data_source *DataSource();

  static ma_result GetDataFormat(ma_data_source *data_source,
                                 ma_format *format, ma_uint32 *channels,
                                 ma_uint32 *sample_rate, ma_channel *channel_map,
                                 size_t channel_map_cap);
  static ma_result Read(ma_data_source *data_source, void *frames_out,
                        ma_uint64 frame_count, ma_uint64 *frames_read);

private:
  ma_data_source_base data_source_{};
  std::unique_ptr<bgm::Track> track_;
  std::string title_;
  bool initialized_ = false;
};

namespace {

constexpr ma_data_source_vtable kWaveformVtable = {
    .onRead = &WaveformSource::Read,
    .onGetDataFormat = &WaveformSource::GetDataFormat,
};

} // namespace

WaveformSource::~WaveformSource() { Unload(); }

AudioResult WaveformSource::Load(std::string_view path) {
  Unload();
  track_ = bgm::OpenTrack(path);
  if (!track_) {
    return AudioResult::Fail(AudioError::TrackOpenFailed,
                             "Failed to open waveform track");
  }
  title_ = track_->metadata.title;

  ma_data_source_config config = ma_data_source_config_init();
  config.vtable = &kWaveformVtable;
  if (ma_data_source_init(&config, &data_source_) != MA_SUCCESS) {
    track_.reset();
    title_.clear();
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize waveform data source");
  }
  initialized_ = true;
  return AudioResult::Ok();
}

void WaveformSource::Unload() {
  if (initialized_) {
    ma_data_source_uninit(&data_source_);
    initialized_ = false;
  }
  track_.reset();
  title_.clear();
}

void WaveformSource::FadeOut(float from,
                             std::chrono::milliseconds duration) {
  if (track_) {
    track_->FadeOut(from, duration);
  }
}

bool WaveformSource::IsLoaded() const {
  return initialized_ && track_ != nullptr;
}

std::string_view WaveformSource::Title() const { return title_; }

float WaveformSource::FadeVolumeLinear() const {
  return track_ ? track_->FadeVolumeLinear() : 0.0f;
}

std::optional<float> WaveformSource::GainFactor() const {
  if (!track_ || !track_->metadata.gain_factor) {
    return std::nullopt;
  }
  return track_->metadata.gain_factor;
}

ma_data_source *WaveformSource::DataSource() {
  return reinterpret_cast<ma_data_source *>(&data_source_);
}

ma_result WaveformSource::GetDataFormat(ma_data_source *data_source,
                                        ma_format *format,
                                        ma_uint32 *channels,
                                        ma_uint32 *sample_rate,
                                        ma_channel *channel_map,
                                        size_t channel_map_cap) {
  auto *self = reinterpret_cast<WaveformSource *>(data_source);
  if (!self->track_) {
    return MA_UNAVAILABLE;
  }
  switch (self->track_->pcmf.format) {
  case PcmSampleFormat::Int16:
    *format = ma_format_s16;
    break;
  case PcmSampleFormat::Int32:
    *format = ma_format_s32;
    break;
  }
  *channels = self->track_->pcmf.channels;
  *sample_rate = self->track_->pcmf.samplingrate;
  (void)channel_map;
  (void)channel_map_cap;
  return MA_SUCCESS;
}

ma_result WaveformSource::Read(ma_data_source *data_source, void *frames_out,
                               ma_uint64 frame_count,
                               ma_uint64 *frames_read) {
  auto *self = reinterpret_cast<WaveformSource *>(data_source);
  if (!self->track_) {
    return MA_UNAVAILABLE;
  }
  const auto sample_size = self->track_->pcmf.SampleSize();
  if ((frame_count > (std::numeric_limits<std::size_t>::max() /
                      sample_size))) {
    return MA_TOO_BIG;
  }
  const auto buffer_size =
      static_cast<std::size_t>(frame_count * sample_size);
  if (!self->track_->Decode(
          {static_cast<std::byte *>(frames_out), buffer_size})) {
    return MA_INVALID_DATA;
  }
  *frames_read = frame_count;
  return MA_SUCCESS;
}

WaveformPlayback::WaveformPlayback(ma_engine &engine) : engine_(engine) {}

WaveformPlayback::~WaveformPlayback() { Unload(); }

AudioResult WaveformPlayback::Load(std::string_view path) {
  Unload();

  auto source = std::make_unique<WaveformSource>();
  auto result = source->Load(path);
  if (!result.success) {
    return result;
  }

  if (ma_sound_init_from_data_source(
          &engine_, source->DataSource(), MA_SOUND_FLAG_NO_SPATIALIZATION,
          nullptr, &sound_) != MA_SUCCESS) {
    return AudioResult::Fail(AudioError::BackendFailed,
                             "Failed to initialize waveform sound");
  }
  source_ = std::move(source);
  sound_initialized_ = true;
  return AudioResult::Ok();
}

void WaveformPlayback::Unload() {
  if (sound_initialized_) {
    ma_sound_uninit(&sound_);
    sound_initialized_ = false;
  }
  source_.reset();
}

void WaveformPlayback::Play() {
  if (IsLoaded()) {
    ma_sound_start(&sound_);
  }
}

void WaveformPlayback::Stop() {
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
}

void WaveformPlayback::Pause() {
  if (IsLoaded()) {
    ma_sound_stop(&sound_);
  }
}

void WaveformPlayback::Resume() {
  if (IsLoaded()) {
    ma_sound_start(&sound_);
  }
}

void WaveformPlayback::SetVolume(float linear) {
  if (IsLoaded()) {
    ma_sound_set_volume(&sound_, linear);
  }
}

void WaveformPlayback::SetPitch(float factor) {
  if (IsLoaded()) {
    ma_sound_set_pitch(&sound_, factor);
  }
}

void WaveformPlayback::FadeOut(float from,
                               std::chrono::milliseconds duration) {
  if (IsLoaded()) {
    source_->FadeOut(from, duration);
  }
}

bool WaveformPlayback::IsLoaded() const {
  return sound_initialized_ && source_ && source_->IsLoaded();
}

std::string_view WaveformPlayback::Title() const {
  return source_ ? source_->Title() : std::string_view{};
}

std::chrono::milliseconds WaveformPlayback::PlayTime() const {
  if (!IsLoaded()) {
    return std::chrono::milliseconds::zero();
  }
  return std::chrono::milliseconds{
      ma_sound_get_time_in_milliseconds(&sound_)};
}

float WaveformPlayback::FadeVolumeLinear() const {
  return source_ ? source_->FadeVolumeLinear() : 0.0f;
}

std::optional<float> WaveformPlayback::GainFactor() const {
  return source_ ? source_->GainFactor() : std::nullopt;
}

} // namespace audio::bgm
