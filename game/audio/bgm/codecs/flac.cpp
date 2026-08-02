///
/// FLAC streaming support using miniaudio's decoder backend.
///

#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <limits>
#include <memory>

#include <miniaudio.h>
#include <span>
#include <utility>

#include "audio/bgm/pcm_source.h"

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

ma_result FlacSeek(ma_decoder *decoder, ma_int64 offset,
                   ma_seek_origin origin) {
  auto &stream = *static_cast<std::istream *>(decoder->pUserData);
  std::ios_base::seekdir whence = 0;
  switch (origin) {
  case ma_seek_origin_start:
    whence = std::ios::beg;
    break;
  case ma_seek_origin_current:
    whence = std::ios::cur;
    break;
  case ma_seek_origin_end:
    whence = std::ios::end;
    break;
  }
  stream.clear();
  stream.seekg(offset, whence);
  return stream ? MA_SUCCESS : MA_BAD_SEEK;
}

struct FlacPcmPart : public PcmPart {
  std::unique_ptr<ma_decoder> decoder;

  size_t PartDecodeSingle(std::span<std::byte> buf) override;
  void PartSeekToSample(size_t sample) override;

  FlacPcmPart(std::unique_ptr<ma_decoder> decoder, const PcmFormat &pcmf)
      : PcmPart(pcmf), decoder(std::move(decoder)) {}
  FlacPcmPart(const FlacPcmPart &) = delete;
  FlacPcmPart &operator=(const FlacPcmPart &) = delete;
  FlacPcmPart(FlacPcmPart &&) = delete;
  FlacPcmPart &operator=(FlacPcmPart &&) = delete;
  ~FlacPcmPart() override;
};

size_t FlacPcmPart::PartDecodeSingle(std::span<std::byte> buf) {
  const auto sample_size = pcmf.SampleSize();
  const auto frames = (buf.size_bytes() / sample_size);
  ma_uint64 frames_read = 0;
  const auto result = ma_decoder_read_pcm_frames(decoder.get(), buf.data(),
                                                 frames, &frames_read);
  if ((result != MA_SUCCESS) && (result != MA_AT_END)) {
    return static_cast<size_t>(-1);
  }
  return static_cast<size_t>(frames_read * sample_size);
}

void FlacPcmPart::PartSeekToSample(size_t sample) {
  ma_decoder_seek_to_pcm_frame(decoder.get(), sample);
}

FlacPcmPart::~FlacPcmPart() { ma_decoder_uninit(decoder.get()); }

} // namespace

std::unique_ptr<PcmPart> OpenFlac(std::istream &stream) {
  auto decoder = std::make_unique<ma_decoder>();
  const auto config = ma_decoder_config_init_default();
  if (ma_decoder_init(FlacRead, FlacSeek, &stream, &config, decoder.get()) !=
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
  return std::make_unique<FlacPcmPart>(std::move(decoder), pcmf);
}

} // namespace audio::bgm
