///
/// FLAC streaming support using miniaudio's decoder backend.
///

#include <cstddef>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <utility>

#include <miniaudio.h>

#include "codec_decoder.h"

namespace audio::bgm {

namespace {

ma_result FlacRead(ma_decoder *decoder, void *buf, size_t size,
                   size_t *bytes_read) {
  if (static_cast<uintmax_t>(size) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return MA_INVALID_ARGS;
  }
  auto &stream = *static_cast<std::istream *>(decoder->pUserData);
  stream.read(static_cast<char *>(buf), static_cast<std::streamsize>(size));
  *bytes_read = static_cast<size_t>(stream.gcount());
  return MA_SUCCESS;
}

} // namespace

std::unique_ptr<PcmPart> OpenFlac(std::istream &stream) {
  auto decoder = std::make_unique<ma_decoder>();
  const auto config = ma_decoder_config_init_default();
  if (ma_decoder_init(FlacRead, StreamSeek, &stream, &config, decoder.get()) !=
      MA_SUCCESS) {
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

  PcmSampleFormat sample_format = PcmSampleFormat::Int16;
  switch (format) {
  case ma_format_s16:
    sample_format = PcmSampleFormat::Int16;
    break;
  case ma_format_s32:
    sample_format = PcmSampleFormat::Int32;
    break;
  default:
    ma_decoder_uninit(decoder.get());
    return nullptr;
  }

  PcmFormat const pcmf = {.samplingrate = sample_rate,
                          .channels = static_cast<uint16_t>(channels),
                          .format = sample_format};
  return std::make_unique<MaDecoderPart>(std::move(decoder), pcmf);
}

} // namespace audio::bgm