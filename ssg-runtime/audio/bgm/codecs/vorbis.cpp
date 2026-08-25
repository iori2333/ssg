///
/// Vorbis streaming support using miniaudio's built-in decoder backend.
///

#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <utility>

#define STB_VORBIS_INCLUDE_STB_VORBIS_H
#include <miniaudio.h>

#include "codec_decoder.h"

namespace audio::bgm {

namespace {

ma_result VorbisRead(ma_decoder *decoder, void *buf, size_t size,
                     size_t *bytes_read) {
  if (static_cast<uintmax_t>(size) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return MA_INVALID_ARGS;
  }
  auto &stream = *static_cast<std::istream *>(decoder->pUserData);
  stream.read(static_cast<char *>(buf), static_cast<std::streamsize>(size));
  *bytes_read = static_cast<size_t>(stream.gcount());
  if (stream.eof()) {
    return MA_AT_END;
  }
  if (*bytes_read < size) {
    return MA_IO_ERROR;
  }
  return MA_SUCCESS;
}

} // namespace

std::unique_ptr<PcmPart> OpenVorbis(std::istream &stream) {
  auto decoder = std::make_unique<ma_decoder>();
  const auto config = ma_decoder_config_init(ma_format_s16, 0, 0);
  if (ma_decoder_init(VorbisRead, StreamSeek, &stream, &config,
                      decoder.get()) != MA_SUCCESS) {
    return nullptr;
  }

  ma_format format = ma_format_unknown;
  ma_uint32 channels = 0;
  ma_uint32 sample_rate = 0;
  if (ma_decoder_get_data_format(decoder.get(), &format, &channels,
                                 &sample_rate, nullptr, 0) != MA_SUCCESS) {
    ma_decoder_uninit(decoder.get());
    return nullptr;
  }
  if (format != ma_format_s16) {
    ma_decoder_uninit(decoder.get());
    return nullptr;
  }

  PcmFormat const pcmf = {.samplingrate = sample_rate,
                          .channels = static_cast<uint16_t>(channels),
                          .format = PcmSampleFormat::Int16};
  return std::make_unique<MaDecoderPart>(std::move(decoder), pcmf);
}

} // namespace audio::bgm