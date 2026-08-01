#include "waveform_source.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <span>

#include <miniaudio.h>

#include "audio/core/audio_types.h"

namespace audio::stream {
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

bool WaveformSource::IsLoaded() const {
  return initialized_ && track_ != nullptr;
}

std::string_view WaveformSource::Title() const { return title_; }

::bgm::Track &WaveformSource::Track() { return *track_; }

const ::bgm::Track &WaveformSource::Track() const { return *track_; }

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

} // namespace audio::stream
