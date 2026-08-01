///
/// Vorbis streaming support (adapted from thcrap's bgmmod module)
///

#include <cassert>
#include <istream>
#include <limits>

// GCC 15 throws an internal compiler error if this appears after a module
// import.
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

#include "audio/bgm_track.h"

namespace bgm {

// Callbacks
// ---------

static size_t CB_Vorbis_Read(void *ptr, size_t size, size_t nmemb,
                             void *datasource) {
  if (size == 0 || nmemb == 0) {
    return 0;
  }
  if (nmemb > std::numeric_limits<size_t>::max() / size) {
    return 0;
  }
  const auto byte_count = size * nmemb;
  if (static_cast<uintmax_t>(byte_count) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return 0;
  }
  auto &stream = *static_cast<std::istream *>(datasource);
  stream.read(static_cast<char *>(ptr),
              static_cast<std::streamsize>(byte_count));
  return static_cast<size_t>(stream.gcount()) / size;
}

int CB_Vorbis_Seek(void *datasource, ogg_int64_t offset, int ov_whence) {
  auto &stream = *static_cast<std::istream *>(datasource);

#pragma warning(suppress : 26494) // type.5
  std::ios_base::seekdir whence;
  switch (ov_whence) {
  case 0:
    whence = std::ios::beg;
    break;
  case 1:
    whence = std::ios::cur;
    break;
  case 2:
    whence = std::ios::end;
    break;
  default:
    assert(!"Invalid seek origin?");
    return -1;
  }
  if (offset < std::numeric_limits<std::streamoff>::min() ||
      offset > std::numeric_limits<std::streamoff>::max()) {
    return -1;
  }
  stream.clear();
  stream.seekg(static_cast<std::streamoff>(offset), whence);
  return stream ? 0 : -1;
}

long CB_Vorbis_Tell(void *datasource) {
  auto &stream = *static_cast<std::istream *>(datasource);
  const std::streamoff offset = stream.tellg();
  if (offset < 0 || offset > std::numeric_limits<long>::max()) {
    return -1;
  }
  return static_cast<long>(offset);
}

static const ov_callbacks kVorbisCallbacks = {
    CB_Vorbis_Read,
    CB_Vorbis_Seek,
    nullptr,
    CB_Vorbis_Tell,
};
// ---------

struct VorbisPcmPart : public bgm::PcmPart {
  OggVorbis_File vf;

  size_t PartDecodeSingle(std::span<std::byte> buf) override;
  void PartSeekToSample(size_t sample) override;

  VorbisPcmPart(OggVorbis_File &&vf, const PcmFormat &pcmf)
      : vf(vf), PcmPart(pcmf) {}
  virtual ~VorbisPcmPart();
};

size_t VorbisPcmPart::PartDecodeSingle(std::span<std::byte> buf) {
  assert(pcmf.format == PcmSampleFormat::Int16);
  auto *buf_as_char = reinterpret_cast<char *>(buf.data());
  return ov_read(&vf, buf_as_char, buf.size_bytes(), 0, 2, 1, nullptr);
}

void VorbisPcmPart::PartSeekToSample(size_t sample) {
  const auto ret = ov_pcm_seek(&vf, sample);
  assert(ret == 0);
}

VorbisPcmPart::~VorbisPcmPart() { ov_clear(&vf); }

std::unique_ptr<bgm::PcmPart>
Vorbis_Open(std::istream &stream,
            std::optional<bgm::MetadataCallback> on_metadata) {
  OggVorbis_File vf = {0};
  const auto ret =
      ov_open_callbacks(&stream, &vf, nullptr, 0, kVorbisCallbacks);
  if (ret || !vf.vi) {
    return nullptr;
  }
  assert(vf.vi->rate >= 0);
  assert(vf.vi->channels >= 0);

  if (const auto &metadata_cb = on_metadata) {
    const auto *vc = ov_comment(&vf, -1);
    if (vc) {
      for (decltype(vc->comments) i = 0; i < vc->comments; i++) {
        // Why signed!?
        const auto len = vc->comment_lengths[i];
        if (vc->comment_lengths[i] < 2) {
          continue;
        }
        bgm::OnVorbisComment(*metadata_cb, {
                                               vc->user_comments[i],
                                               static_cast<size_t>(len),
                                           });
      }
    }
  }

  const auto samplingrate = static_cast<uint32_t>(vf.vi->rate);
  const auto channels = static_cast<uint16_t>(vf.vi->channels);
  PcmFormat pcmf = {samplingrate, channels, PcmSampleFormat::Int16};
  return std::make_unique<VorbisPcmPart>(std::move(vf), pcmf);
}

} // namespace bgm
