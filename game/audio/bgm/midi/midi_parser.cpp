#include "midi_parser.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "util/byte_io.h"
#include "util/endian.h"

namespace audio::bgm {
namespace {

#pragma pack(push, 1)
struct SmfFileHeader {
  util::BigEndian<std::uint32_t> magic;
  util::BigEndian<std::uint32_t> size;
};

struct SmfMainHeader {
  util::BigEndian<std::uint16_t> format;
  util::BigEndian<std::uint16_t> track;
  util::BigEndian<std::uint16_t> timebase;
};

struct SmfTrackHeader {
  util::BigEndian<std::uint32_t> magic;
  util::BigEndian<std::uint32_t> size;
};
#pragma pack(pop)

} // namespace

std::optional<SequenceData> ParseMidi(std::span<const std::uint8_t> buffer) {
  util::ByteReader reader{buffer};
  const auto file_header = reader.ReadObject<SmfFileHeader>();
  if (!file_header || file_header->magic != 0x4d546864) {
    return std::nullopt;
  }
  const auto main_header = reader.ReadObject<SmfMainHeader>();
  if (!main_header || main_header->format > 2 || main_header->track == 0 ||
      main_header->timebase == 0) {
    return std::nullopt;
  }

  SequenceData sequence;
  sequence.ppqn = main_header->timebase;
  sequence.tracks.reserve(main_header->track);

  for (std::size_t i = 0; i < main_header->track; i++) {
    const auto track_header = reader.ReadObject<SmfTrackHeader>();
    if (!track_header || track_header->magic != 0x4d54726b) {
      return std::nullopt;
    }
    const auto track_data = reader.ReadBytes(track_header->size);
    if (!track_data) {
      return std::nullopt;
    }
    const auto offset = sequence.storage.size();
    sequence.storage.insert(sequence.storage.end(), track_data->begin(),
                            track_data->end());
    sequence.tracks.push_back({.offset = offset, .size = track_data->size()});
  }
  return sequence;
}

} // namespace audio::bgm
