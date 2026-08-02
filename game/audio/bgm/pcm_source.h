/// Low-level PCM source decoding shared by BGM codecs and PCM track playback.

#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <fstream>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "audio/core/volume_ramp.h"

// PCM sample format
// -----------------

enum class PcmSampleFormat : uint8_t {
  Int16 = 2,
  Int32 = 4,
};

struct PcmFormat {
  uint32_t samplingrate;
  uint16_t channels;
  PcmSampleFormat format;

  std::strong_ordering operator<=>(const PcmFormat &other) const = default;

  [[nodiscard]] size_t SampleSize() const {
    const auto byte_depth = std::to_underlying(format);
    return (static_cast<size_t>(channels) * byte_depth);
  }
};
// -----------------

namespace audio::bgm {

using SampleCount = uint32_t;

class PcmVolume {
  audio::VolumeRamp ramp;

public:
  // Value for a volume control with perceived linear loudness.
  auto FadeVolumeLinear() const { return ramp.Current(); }

  // Multiplication factor for PCM samples.
  auto FadeVolumeFactor() const {
    const auto linear = ramp.Current();
    return (linear * linear);
  }

  void SetVolumeLinear(float v);

  // Advances one PCM frame and returns the new linear volume.
  float NextFrame() { return ramp.NextFrame(); }

  void StartFade(float from, float to, SampleCount frames) {
    ramp.StartFade(from, to, frames);
  }
};
// ----------------------

// PCM source formats
// ----------------------------

// Base class for an individual intro or loop file.
// Should be derived for each supported codec.
struct PcmPart {
  PcmFormat pcmf;

  // Single decoding call. Should return the number of bytes actually decoded
  // (which can be less than [buf.size_bytes()]) or -1 if an error occurred.
  virtual size_t PartDecodeSingle(std::span<std::byte> buf) = 0;

  // Seeks to the given raw decoded audio sample. Guaranteed to be less than
  // the total number of samples in the stream.
  virtual void PartSeekToSample(size_t sample) = 0;

  PcmPart(const PcmFormat &pcmf) : pcmf(pcmf) {}
  PcmPart(const PcmPart &) = delete;
  PcmPart &operator=(const PcmPart &) = delete;
  PcmPart(PcmPart &&) = delete;
  PcmPart &operator=(PcmPart &&) = delete;
  virtual ~PcmPart() = default;
};

// Generic implementation for PCM codecs with separate intro and loop files.
struct PcmStream {
  PcmFormat pcmf;
  PcmVolume vol;
  std::unique_ptr<std::ifstream> intro_stream;
  std::unique_ptr<std::ifstream> loop_stream;
  std::unique_ptr<PcmPart> intro_part;
  std::unique_ptr<PcmPart> loop_part;
  PcmPart *cur;

  // *Always* fills [buf] entirely. Returns `true` if successful, or `false`
  // in case of an unrecoverable decoding error, in which case [buf] is
  // filled with zeroes.
  bool Decode(std::span<std::byte> buf);

  size_t DecodeSingle(std::span<std::byte> buf);

  auto FadeVolumeLinear() const { return vol.FadeVolumeLinear(); }

  // Starts a fade-out that takes the given number of milliseconds.
  void FadeOut(float volume_start, std::chrono::milliseconds duration);

  PcmStream(std::unique_ptr<std::ifstream> intro_stream,
            std::unique_ptr<std::ifstream> loop_stream,
            std::unique_ptr<PcmPart> intro_part,
            std::unique_ptr<PcmPart> loop_part)
      : pcmf(intro_part->pcmf), intro_stream(std::move(intro_stream)),
        loop_stream(std::move(loop_stream)), intro_part(std::move(intro_part)),
        loop_part(std::move(loop_part)), cur(this->intro_part.get()) {}
};

// Tries to opens [stream] as a part of a modded track, using a specific codec.
// `PcmStream` retains ownership of [stream].
using PcmPartOpen = std::unique_ptr<PcmPart>(std::istream &stream);
// ----------------------------

std::unique_ptr<PcmPart> OpenFlac(std::istream &stream);
std::unique_ptr<PcmPart> OpenVorbis(std::istream &stream);

// Tries to open a waveform track whose name starts with [base_fn] and has one
// of the supported codec extensions.
std::unique_ptr<PcmStream> OpenPcmStream(std::string_view base_fn);

} // namespace audio::bgm
