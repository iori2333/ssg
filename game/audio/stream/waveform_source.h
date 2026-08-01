/// Miniaudio streaming data source backed by bgm::Track.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include <miniaudio.h>

#include "audio/bgm_track.h"
#include "audio/core/audio_types.h"

namespace audio::stream {

class WaveformSource {
public:
  WaveformSource() = default;
  ~WaveformSource();
  WaveformSource(const WaveformSource &) = delete;
  WaveformSource &operator=(const WaveformSource &) = delete;

  AudioResult Load(std::string_view path);
  void Unload();

  [[nodiscard]] bool IsLoaded() const;
  [[nodiscard]] std::string_view Title() const;
  static ma_result GetDataFormat(ma_data_source *data_source,
                                 ma_format *format, ma_uint32 *channels,
                                 ma_uint32 *sample_rate, ma_channel *channel_map,
                                 size_t channel_map_cap);
  static ma_result Read(ma_data_source *data_source, void *frames_out,
                        ma_uint64 frame_count, ma_uint64 *frames_read);

  [[nodiscard]] ::bgm::Track &Track();
  [[nodiscard]] const ::bgm::Track &Track() const;
  [[nodiscard]] ma_data_source *DataSource();

private:
  ma_data_source_base data_source_{};
  std::unique_ptr<::bgm::Track> track_;
  std::string title_;
  bool initialized_ = false;
};

} // namespace audio::stream
