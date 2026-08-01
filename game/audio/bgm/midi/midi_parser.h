/// Pure MIDI file parser producing owned sequence data.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace audio::bgm {

struct TrackView {
  std::size_t offset = 0;
  std::size_t size = 0;
};

struct SequenceData {
  std::vector<std::uint8_t> storage;
  std::vector<TrackView> tracks;
  std::uint16_t ppqn = 0;

  [[nodiscard]] std::span<const std::uint8_t> Track(std::size_t index) const {
    const auto &view = tracks[index];
    return {storage.data() + view.offset, view.size};
  }
};

[[nodiscard]] std::optional<SequenceData>
ParseMidi(std::span<const std::uint8_t> buffer);

} // namespace audio::bgm
