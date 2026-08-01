///
/// FLAC streaming support (adapted from thcrap's bgmmod module)
///

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_STDIO

#include <cstdint>
#include <istream>
#include <limits>
#include <memory>

// GCC 15 throws `error: conflicting declaration 'typedef struct max_align_t
// max_align_t'` if this appears after a module import.
#include <dr_flac.h>

#include "audio/bgm/pcm_source.h"

namespace audio::bgm {

// Callbacks
// ---------

struct FlacCallbackData {
  std::istream &stream;

  explicit FlacCallbackData(std::istream &stream) : stream(stream) {}
};

size_t FlacRead(void *user_data, void *buf, size_t size) {
  if (static_cast<uintmax_t>(size) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return 0;
  }
  auto &stream = static_cast<FlacCallbackData *>(user_data)->stream;
  stream.read(static_cast<char *>(buf), static_cast<std::streamsize>(size));
  return static_cast<size_t>(stream.gcount());
}

drflac_bool32 FlacSeek(void *user_data, int offset, drflac_seek_origin origin) {
  auto &stream = static_cast<FlacCallbackData *>(user_data)->stream;
  std::ios_base::seekdir whence;
  switch (origin) {
  case DRFLAC_SEEK_SET:
    whence = std::ios::beg;
    break;
  case DRFLAC_SEEK_CUR:
    whence = std::ios::cur;
    break;
  case DRFLAC_SEEK_END:
    whence = std::ios::end;
    break;
  }
  stream.clear();
  stream.seekg(offset, whence);
  return stream ? DRFLAC_TRUE : DRFLAC_FALSE;
}

drflac_bool32 FlacTell(void *user_data, drflac_int64 *cursor) {
  auto &stream = static_cast<FlacCallbackData *>(user_data)->stream;
  const std::streamoff offset = stream.tellg();
  if (!cursor || offset < 0) {
    return DRFLAC_FALSE;
  }
  *cursor = offset;
  return DRFLAC_TRUE;
}
// ---------

using FlacReadFunction = drflac_uint64(drflac *, drflac_uint64, void *);

struct FlacPcmPart : public PcmPart {
  std::unique_ptr<FlacCallbackData> callback_data;
  drflac *ff;
  FlacReadFunction &read_func;

  size_t PartDecodeSingle(std::span<std::byte> buf) override;
  void PartSeekToSample(size_t sample) override;

  FlacPcmPart(std::unique_ptr<FlacCallbackData> callback_data, drflac *ff,
              const PcmFormat &pcmf, FlacReadFunction &read_func)
      : PcmPart(pcmf), callback_data(std::move(callback_data)), ff(ff),
        read_func(read_func) {}
  ~FlacPcmPart() override;
};

size_t FlacPcmPart::PartDecodeSingle(std::span<std::byte> buf) {
  const auto sample_size = pcmf.SampleSize();
  const auto samples = (buf.size_bytes() / sample_size);
  return (read_func(ff, samples, buf.data()) * sample_size);
}

void FlacPcmPart::PartSeekToSample(size_t sample) {
  drflac_seek_to_pcm_frame(ff, sample);
}

FlacPcmPart::~FlacPcmPart() { drflac_close(ff); }

std::unique_ptr<PcmPart> OpenFlac(std::istream &stream) {
  auto callback_data = std::make_unique<FlacCallbackData>(stream);
  auto *ff =
      drflac_open(FlacRead, FlacSeek, FlacTell, callback_data.get(), nullptr);
  if (!ff) {
    return nullptr;
  }
  const auto output_format =
      ((ff->bitsPerSample <= 16) ? PcmSampleFormat::Int16
                                 : PcmSampleFormat::Int32);
  const auto read_func =
      ((ff->bitsPerSample <= 16)
           ? reinterpret_cast<FlacReadFunction &>(drflac_read_pcm_frames_s16)
           : reinterpret_cast<FlacReadFunction &>(drflac_read_pcm_frames_s32));
  PcmFormat pcmf = {ff->sampleRate, ff->channels, output_format};
  return std::make_unique<FlacPcmPart>(std::move(callback_data), ff, pcmf,
                                       *read_func);
}

} // namespace audio::bgm
