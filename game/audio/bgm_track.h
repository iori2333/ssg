///
/// BGM tracks rendered as PCM and output via the Snd subsystem
///
#pragma once

#include <chrono>
#include <compare>
#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

// PCM sample format
// -----------------

enum class PcmSampleFormat : uint8_t {
  Int16 = 2,
  Int32 = 4,
};

struct PcmFormat {
  const uint32_t samplingrate;
  const uint16_t channels;
  const PcmSampleFormat format;

  std::strong_ordering operator<=>(const PcmFormat &other) const = default;

  size_t SampleSize() const {
    const auto byte_depth = std::to_underlying(format);
    return (channels * byte_depth);
  }
};
// -----------------

namespace bgm {

using SampleCount = uint32_t;

// Metadata
// --------

struct TrackMetadata {
  std::string title;

  // Gain to apply to the track.
  std::optional<float> gain_factor;
};

// Vorbis comments are specified to use UTF-8.
using MetadataCallback =
    std::function<void(std::string_view tag, std::string_view value)>;

// Calls [func] for the given Vorbis comment.
void OnVorbisComment(MetadataCallback func, std::string_view comment);
// --------

// Base class for a track
// ----------------------

class TrackVolume {
  float volume_linear = 1.0f;
  float volume_factor = 1.0f;

public:
  float fade_delta = 0.0f;
  float fade_end = 0.0f;
  SampleCount fade_remaining = 0;
  SampleCount fade_duration = 0;

  // Value for a volume control with perceived linear loudness.
  auto FadeVolumeLinear() const { return volume_linear; }

  // Multiplication factor for PCM samples.
  auto FadeVolumeFactor() const { return volume_factor; }

  void SetVolumeLinear(float v);
};

struct Track {
protected:
  TrackVolume vol;

public:
  const TrackMetadata metadata;

  // Target format that this track is decoded to.
  const PcmFormat pcmf;

  // Single decoding call that implicitly handles looping. Should return the
  // number of bytes actually decoded (which can be less than
  // [buf.size_bytes()]), or -1 if an error occurred.
  virtual size_t DecodeSingle(std::span<std::byte> buf) = 0;

  // *Always* fills [buf] entirely. Returns `true` if successful, or `false`
  // in case of an unrecoverable decoding error, in which case [buf] is
  // filled with zeroes.
  bool Decode(std::span<std::byte> buf);

  auto FadeVolumeLinear() const { return vol.FadeVolumeLinear(); }

  // Starts a fade-out that takes the given number of milliseconds.
  void FadeOut(float volume_start, std::chrono::milliseconds duration);

  Track(TrackMetadata &&metadata, const PcmFormat &pcmf)
      : metadata(metadata), pcmf(pcmf) {}

  virtual ~Track() {}
};
// ----------------------

// Tracks in PCM source formats
// ----------------------------

// Base class for an individual intro or loop file.
// Should be derived for each supported codec.
struct PcmPart {
  const PcmFormat pcmf;

  // Single decoding call. Should return the number of bytes actually decoded
  // (which can be less than [buf.size_bytes()]) or -1 if an error occurred.
  virtual size_t PartDecodeSingle(std::span<std::byte> buf) = 0;

  // Seeks to the given raw decoded audio sample. Guaranteed to be less than
  // the total number of samples in the stream.
  virtual void PartSeekToSample(size_t sample) = 0;

  PcmPart(const PcmFormat &pcmf) : pcmf(pcmf) {}
  virtual ~PcmPart() {}
};

// Generic implementation for PCM-based codecs, with separate intro and loop
// files.
struct PcmTrack : public Track {
  std::unique_ptr<std::ifstream> intro_stream;
  std::unique_ptr<std::ifstream> loop_stream;
  std::unique_ptr<PcmPart> intro_part;
  std::unique_ptr<PcmPart> loop_part;
  PcmPart *cur;

  virtual size_t DecodeSingle(std::span<std::byte> buf) override;

  PcmTrack(TrackMetadata &&metadata,
           std::unique_ptr<std::ifstream> intro_stream,
           std::unique_ptr<std::ifstream> loop_stream,
           std::unique_ptr<PcmPart> intro_part,
           std::unique_ptr<PcmPart> loop_part)
      : Track(std::move(metadata), intro_part->pcmf),
        intro_stream(std::move(intro_stream)),
        loop_stream(std::move(loop_stream)), intro_part(std::move(intro_part)),
        loop_part(std::move(loop_part)), cur(this->intro_part.get()) {}
};

// Tries to opens [stream] as a part of a modded track, using a specific codec.
// `PcmTrack` retains ownership of [stream].
using PcmPartOpen = std::unique_ptr<PcmPart>(
    std::istream &stream, std::optional<MetadataCallback> on_metadata);
// ----------------------------

// Tries to open a waveform track whose name starts with [base_fn] and has one
// of the supported codec extensions.
std::unique_ptr<Track> OpenTrack(std::string_view base_fn);

} // namespace bgm
