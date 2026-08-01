/// Low-level PCM source decoding shared by BGM codecs and PCM track playback
/// (adapted from thcrap's bgmmod module).

#include <algorithm>
#include <cassert>
#include <fstream>

#include "pcm_source.h"

namespace audio::bgm {

void ApplyVolumeError(std::span<std::byte>, uint16_t, PcmVolume &) {
  assert(!"missing fade implementation");
}

void PcmVolume::SetVolumeLinear(float v) { ramp.Set(v); }

template <typename BitDepth>
static void ApplyVolume(std::span<std::byte> buf, uint16_t channels,
                        PcmVolume &vol) {
  const auto samples =
      std::span<BitDepth>{reinterpret_cast<BitDepth *>(buf.data()),
                          (buf.size_bytes() / sizeof(BitDepth))};
  auto it = samples.begin();

  while (it != samples.end()) {
    for (decltype(channels) c = 0; c < channels; c++) {
      *(it++) *= vol.FadeVolumeFactor();
    }
    vol.NextFrame();
  }
}

bool PcmStream::Decode(std::span<std::byte> buf) {
  size_t offset = 0;
  auto size_left = buf.size_bytes();
  while (size_left > 0) {
    const auto ret = DecodeSingle(buf.subspan(offset, size_left));
    if ((ret == static_cast<size_t>(-1)) || (ret > size_left)) {
      std::ranges::fill(buf, std::byte{0});
      return false;
    }
    offset += ret;
    size_left -= ret;
  }

  const auto apply_volume =
      ((pcmf.format == PcmSampleFormat::Int16)   ? ApplyVolume<int16_t>
       : (pcmf.format == PcmSampleFormat::Int32) ? ApplyVolume<int32_t>
                                                 : ApplyVolumeError);
  apply_volume(buf, pcmf.channels, vol);
  return true;
}

void PcmStream::FadeOut(float volume_start,
                        std::chrono::milliseconds duration) {
  const auto sample_count = ((duration.count() * pcmf.samplingrate) / 1000);
  vol.StartFade(volume_start, 0.0f, sample_count);
}

size_t PcmStream::DecodeSingle(std::span<std::byte> buf) {
  const auto ret = cur->PartDecodeSingle(buf);
  if (ret == 0) {
    if ((cur == intro_part.get()) && (loop_part.get() != nullptr)) {
      cur = loop_part.get();
    }
    cur->PartSeekToSample(0);
  }
  return ret;
}

// Codecs
// ------

struct PcmCodec {
  std::string_view ext;
  PcmPartOpen &open;
};

std::unique_ptr<PcmPart> OpenFlac(std::istream &stream);
std::unique_ptr<PcmPart> OpenVorbis(std::istream &stream);

// Sorted in order of preference.
constexpr PcmCodec kPcmCodecs[] = {
    {".flac", OpenFlac},
    {".ogg", OpenVorbis},
};
// ------

static constexpr std::string_view kLoopInfix = ".loop";
static constexpr size_t kExtensionCapacity =
    std::ranges::max_element(kPcmCodecs, [](const auto &a, const auto &b) {
      return (a.ext.size() < b.ext.size());
    })->ext.size();

std::unique_ptr<PcmStream> OpenPcmStream(std::string_view base_fn) {
  const size_t base_len = base_fn.size();
  const size_t fn_len = (kLoopInfix.size() + kExtensionCapacity);
  std::string fn;
  fn.resize_and_overwrite((base_len + fn_len), [&](char *buf, size_t len) {
    std::ranges::copy(base_fn, buf);
    return base_len;
  });
  for (const auto &codec : kPcmCodecs) {
    fn.resize(base_len);
    fn += codec.ext;
    auto intro_stream = std::make_unique<std::ifstream>(fn, std::ios::binary);
    if (!*intro_stream) {
      continue;
    }

    std::unique_ptr<PcmPart> intro_part;
    std::unique_ptr<PcmPart> loop_part;

    intro_part = codec.open(*intro_stream);
    if (!intro_part) {
      continue;
    }

    fn.resize(base_len);
    fn += kLoopInfix;
    fn += codec.ext;
    auto loop_stream = std::make_unique<std::ifstream>(fn, std::ios::binary);
    if (*loop_stream) {
      loop_part = codec.open(*loop_stream);
      if (!loop_part) {
        continue;
      }
      if (intro_part->pcmf != loop_part->pcmf) {
        assert(!"PCM format mismatch between intro and loop parts!");
        loop_part.reset();
        continue;
      }
    } else {
      loop_stream.reset();
    }
    return std::make_unique<PcmStream>(
        std::move(intro_stream), std::move(loop_stream), std::move(intro_part),
        std::move(loop_part));
  }
  return nullptr;
}

} // namespace audio::bgm
