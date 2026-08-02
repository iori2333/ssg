/// Low-level PCM source decoding shared by BGM codecs and PCM track playback
/// (adapted from thcrap's bgmmod module).

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "pcm_source.h"

namespace audio::bgm {

namespace {

void ApplyVolumeError(std::span<uint8_t> /*unused*/, uint16_t /*unused*/,
                      PcmVolume & /*unused*/) {
  assert(!"missing fade implementation");
}

} // namespace

void PcmVolume::SetVolumeLinear(float v) { ramp.Set(v); }

namespace {

template <typename BitDepth>
void ApplyVolume(std::span<uint8_t> buf, uint16_t channels, PcmVolume &vol) {
  const auto samples =
      std::span<BitDepth>{reinterpret_cast<BitDepth *>(buf.data()),
                          (buf.size() / sizeof(BitDepth))};
  auto it = samples.begin();

  while (it != samples.end()) {
    for (decltype(channels) c = 0; c < channels; c++) {
      *(it++) *= vol.FadeVolumeFactor();
    }
    vol.NextFrame();
  }
}

} // namespace

bool PcmStream::Decode(std::span<uint8_t> buf) {
  size_t offset = 0;
  auto size_left = buf.size();
  while (size_left > 0) {
    const auto ret = DecodeSingle(buf.subspan(offset, size_left));
    if (ret == 0) {
      std::ranges::fill(buf.subspan(offset, size_left), uint8_t{0});
      return true;
    }
    if ((std::cmp_equal(ret, -1)) || (ret > size_left)) {
      std::ranges::fill(buf, uint8_t{0});
      return false;
    }
    offset += ret;
    size_left -= ret;
  }

  auto apply_volume = ApplyVolumeError;
  if (pcmf.format == PcmSampleFormat::Int16) {
    apply_volume = ApplyVolume<int16_t>;
  } else if (pcmf.format == PcmSampleFormat::Int32) {
    apply_volume = ApplyVolume<int32_t>;
  }
  apply_volume(buf, pcmf.channels, vol);
  return true;
}

void PcmStream::FadeOut(float volume_start,
                        std::chrono::milliseconds duration) {
  const auto sample_count = ((duration.count() * pcmf.samplingrate) / 1000);
  vol.StartFade(volume_start, 0.0F, sample_count);
}

size_t PcmStream::DecodeSingle(std::span<uint8_t> buf) {
  const auto ret = cur->PartDecodeSingle(buf);
  if (ret == 0) {
    if ((cur == intro_part.get()) && (loop_part != nullptr)) {
      cur = loop_part.get();
    }
    cur->PartSeekToSample(0);
  }
  return ret;
}

// Codecs
// ------

namespace {

struct PcmCodec {
  std::string_view ext;
  PcmPartOpen &open;
};

// Sorted in order of preference.
constexpr std::array<PcmCodec, 2> kPcmCodecs = {
    PcmCodec{.ext = ".flac", .open = OpenFlac},
    PcmCodec{.ext = ".ogg", .open = OpenVorbis},
};
// ------

constexpr std::string_view kLoopInfix = ".loop";
constexpr size_t kExtensionCapacity =
    std::ranges::max_element(kPcmCodecs, [](const auto &a, const auto &b) {
      return (a.ext.size() < b.ext.size());
    })->ext.size();

} // namespace

std::unique_ptr<PcmStream> OpenPcmStream(std::string_view base_fn) {
  const size_t base_len = base_fn.size();
  const size_t fn_len = (kLoopInfix.size() + kExtensionCapacity);
  std::string fn;
  fn.resize_and_overwrite((base_len + fn_len), [&](char *buf, size_t /*len*/) {
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
