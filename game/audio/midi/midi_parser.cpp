#include "midi_parser.h"

#include <cstdint>
#include <optional>
#include <string>

#include "util/byte_io.h"
#include "util/endian.h"

namespace audio::midi {
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

std::optional<std::uint32_t> ConsumeVlq(util::ByteReader &reader) {
  std::uint32_t ret = 0;
  for (std::size_t i = 0; i < 4; i++) {
    const auto byte = reader.Read<std::uint8_t>();
    if (!byte) {
      return std::nullopt;
    }
    ret = ((ret << 7) | (*byte & 0x7f));
    if ((*byte & 0x80) == 0) {
      return ret;
    }
  }
  return std::nullopt;
}

std::optional<std::string> FindTitle(std::span<const std::uint8_t> data) {
  util::ByteReader reader{data};
  std::uint8_t status = 0;
  while (const auto status_byte = reader.Peek<std::uint8_t>()) {
    if (*status_byte >= 0x80) {
      status = *reader.Read<std::uint8_t>();
    }

    if (status < 0x80) {
      return std::nullopt;
    }

    if (status == 0xff) {
      const auto meta = reader.Read<std::uint8_t>();
      const auto length = ConsumeVlq(reader);
      if (!meta || !length) {
        return std::nullopt;
      }
      const auto text = reader.ReadBytes(*length);
      if (!text) {
        return std::nullopt;
      }
      if ((*meta == 0x03 || *meta == 0x01) && !text->empty()) {
        return std::string{
            reinterpret_cast<const char *>(text->data()), text->size()};
      }
      continue;
    }

    if (status == 0xf0 || status == 0xf7) {
      const auto length = ConsumeVlq(reader);
      if (!length || !reader.ReadBytes(*length)) {
        return std::nullopt;
      }
      continue;
    }

    switch (status & 0xf0) {
    case 0x80:
    case 0x90:
    case 0xa0:
    case 0xb0:
    case 0xe0:
      if (!reader.ReadBytes(2)) {
        return std::nullopt;
      }
      break;
    case 0xc0:
    case 0xd0:
      if (!reader.ReadBytes(1)) {
        return std::nullopt;
      }
      break;
    default:
      return std::nullopt;
    }
  }
  return std::nullopt;
}

} // namespace

std::optional<SequenceData> ParseMidi(
    std::span<const std::uint8_t> buffer) {
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

  if (sequence.title.empty()) {
    for (std::size_t i = 0; i < sequence.tracks.size(); i++) {
      if (auto title = FindTitle(sequence.Track(i))) {
        sequence.title = std::move(*title);
        break;
      }
    }
  }
  return sequence;
}

} // namespace audio::midi

