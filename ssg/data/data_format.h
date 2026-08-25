///
/// Shared data-format helpers used by both the runtime loader
/// (ssg/data/game_data.cpp) and pack_tool, so format rules cannot drift.
///

#pragma once

#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>

namespace data {

[[nodiscard]] inline bool IsStandardMidi(std::span<const uint8_t> data) {
  static constexpr std::array<uint8_t, 8> kHeader = {'M', 'T', 'h', 'd',
                                                     0,   0,   0,   6};
  return data.size() >= 14 &&
         std::ranges::equal(data.first(kHeader.size()), kHeader);
}

[[nodiscard]] inline bool IsBitmap(std::span<const uint8_t> data) {
  return data.size() >= 14 && data[0] == 'B' && data[1] == 'M';
}

[[nodiscard]] inline bool IsWave(std::span<const uint8_t> data) {
  return data.size() >= 12 && data[0] == 'R' && data[1] == 'I' &&
         data[2] == 'F' && data[3] == 'F' && data[8] == 'W' && data[9] == 'A' &&
         data[10] == 'V' && data[11] == 'E';
}

// Parses a zero-padded numeric entry stem (e.g. "012") with the expected
// extension into its index.
[[nodiscard]] inline std::optional<uint32_t>
ParseEntryIndex(const std::filesystem::path &path, std::string_view extension) {
  if (path.extension() != extension) {
    return std::nullopt;
  }
  const auto stem = path.stem().string();
  uint32_t index = 0;
  const auto [end, error] =
      std::from_chars(stem.data(), stem.data() + stem.size(), index);
  if (stem.empty() || error != std::errc{} ||
      end != stem.data() + stem.size()) {
    return std::nullopt;
  }
  return index;
}

} // namespace data