///
/// Vorbis streaming support using miniaudio's built-in decoder backend.
///

#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <limits>
#include <memory>
#include <span>
#include <utility>

#define STB_VORBIS_INCLUDE_STB_VORBIS_H
#include <miniaudio.h>

#include "audio/bgm/pcm_source.h"

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

ma_result VorbisSeek(ma_decoder *decoder, ma_int64 offset,
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

struct VorbisPcmPart : public PcmPart {
  std::unique_ptr<ma_decoder> decoder;

  size_t PartDecodeSingle(std::span<uint8_t> buf) override;
  void PartSeekToSample(size_t sample) override;

  VorbisPcmPart(std::unique_ptr<ma_decoder> decoder, const PcmFormat &pcmf)
      : PcmPart(pcmf), decoder(std::move(decoder)) {}
  VorbisPcmPart(const VorbisPcmPart &) = delete;
  VorbisPcmPart &operator=(const VorbisPcmPart &) = delete;
  VorbisPcmPart(VorbisPcmPart &&) = delete;
  VorbisPcmPart &operator=(VorbisPcmPart &&) = delete;
  ~VorbisPcmPart() override;
};

size_t VorbisPcmPart::PartDecodeSingle(std::span<uint8_t> buf) {
  const auto sample_size = pcmf.SampleSize();
  const auto frames = (buf.size() / sample_size);
  ma_uint64 frames_read = 0;
  const auto result = ma_decoder_read_pcm_frames(decoder.get(), buf.data(),
                                                 frames, &frames_read);
  if ((result != MA_SUCCESS) && (result != MA_AT_END)) {
    return static_cast<size_t>(-1);
  }
  return static_cast<size_t>(frames_read * sample_size);
}

void VorbisPcmPart::PartSeekToSample(size_t sample) {
  ma_decoder_seek_to_pcm_frame(decoder.get(), sample);
}

VorbisPcmPart::~VorbisPcmPart() { ma_decoder_uninit(decoder.get()); }

} // namespace

std::unique_ptr<PcmPart> OpenVorbis(std::istream &stream) {
  auto decoder = std::make_unique<ma_decoder>();
  const auto config = ma_decoder_config_init(ma_format_s16, 0, 0);
  if (ma_decoder_init(VorbisRead, VorbisSeek, &stream, &config,
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
  return std::make_unique<VorbisPcmPart>(std::move(decoder), pcmf);
}

} // namespace audio::bgm
