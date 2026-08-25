///
/// Shared miniaudio decoder plumbing for streaming codecs (FLAC, Vorbis).
/// Each codec supplies its own read callback and decoder config; the seek
/// callback and PCM-part wrapper are shared here.
///

#pragma once

#include <cstddef>
#include <cstdint>
#include <ios>
#include <istream>
#include <memory>
#include <span>
#include <utility>

#include <miniaudio.h>

#include "audio/bgm/pcm_source.h"

namespace audio::bgm {

// Stream-backed seek callback shared by all decoder codecs.
inline ma_result StreamSeek(ma_decoder *decoder, ma_int64 offset,
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

// A PcmPart that decodes through an owning ma_decoder.
class MaDecoderPart : public PcmPart {
public:
  MaDecoderPart(std::unique_ptr<ma_decoder> decoder, const PcmFormat &pcmf)
      : PcmPart(pcmf), decoder_(std::move(decoder)) {}
  MaDecoderPart(const MaDecoderPart &) = delete;
  MaDecoderPart &operator=(const MaDecoderPart &) = delete;
  MaDecoderPart(MaDecoderPart &&) = delete;
  MaDecoderPart &operator=(MaDecoderPart &&) = delete;
  ~MaDecoderPart() override { ma_decoder_uninit(decoder_.get()); }

  size_t PartDecodeSingle(std::span<uint8_t> buf) override {
    const auto sample_size = pcmf.SampleSize();
    const auto frames = (buf.size() / sample_size);
    ma_uint64 frames_read = 0;
    const auto result = ma_decoder_read_pcm_frames(decoder_.get(), buf.data(),
                                                   frames, &frames_read);
    if ((result != MA_SUCCESS) && (result != MA_AT_END)) {
      return static_cast<size_t>(-1);
    }
    return static_cast<size_t>(frames_read * sample_size);
  }

  void PartSeekToSample(size_t sample) override {
    ma_decoder_seek_to_pcm_frame(decoder_.get(), sample);
  }

private:
  std::unique_ptr<ma_decoder> decoder_;
};

} // namespace audio::bgm