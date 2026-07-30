///
/// DataManifest - versioned section map for the unified DATA.PAK archive
///
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace data {

enum class DataSectionId : uint32_t {
  Maps,
  Images,
  Music,
  Sounds,
  Demos,
  Count,
};

inline constexpr std::array<std::string_view,
                            std::to_underlying(DataSectionId::Count)>
    kDataSectionNames = {"maps", "images", "music", "sounds", "demos"};

inline constexpr std::array<std::string_view,
                            std::to_underlying(DataSectionId::Count)>
    kDataSectionExtensions = {".map", ".bmp", ".mid", ".wav", ".dat"};

struct DataSectionRange {
  uint32_t first_entry = 0;
  uint32_t entry_count = 0;
};

struct DataManifest {
  std::array<DataSectionRange, std::to_underlying(DataSectionId::Count)>
      sections{};
};

[[nodiscard]] std::optional<DataManifest>
ParseDataManifest(std::span<const uint8_t> bytes, uint32_t archive_entry_count);

[[nodiscard]] std::vector<uint8_t> BuildDataManifest(
    const std::array<uint32_t, std::to_underlying(DataSectionId::Count)>
        &section_counts);

} // namespace data
