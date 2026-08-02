///
/// DataManifest - versioned section map for the unified DATA.PAK archive
///

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "data_manifest.h"

#include "util/byte_io.h"

namespace data {

namespace {

constexpr std::array<uint8_t, 8> kMagic = {'S', 'S', 'G', 'D',
                                           'A', 'T', 'A', 0x1a};
constexpr uint32_t kVersion = 3;

} // namespace

std::optional<DataManifest> ParseDataManifest(std::span<const uint8_t> bytes,
                                              uint32_t archive_entry_count) {
  util::ByteReader reader{bytes};
  const auto magic = reader.ReadBytes(kMagic.size());
  const auto version = reader.Read<uint32_t>();
  const auto section_count = reader.Read<uint32_t>();
  if (!magic || !std::ranges::equal(*magic, kMagic) || !version ||
      *version != kVersion || !section_count ||
      *section_count != std::to_underlying(DataSectionId::Count) ||
      archive_entry_count == 0) {
    return std::nullopt;
  }

  DataManifest manifest;
  std::array<bool, std::to_underlying(DataSectionId::Count)> seen_sections{};
  std::vector<bool> seen_entries(archive_entry_count);
  seen_entries[0] = true;
  for (uint32_t i = 0; i < *section_count; ++i) {
    const auto raw_id = reader.Read<uint32_t>();
    const auto first_entry = reader.Read<uint32_t>();
    const auto entry_count = reader.Read<uint32_t>();
    if (!raw_id || *raw_id >= seen_sections.size() || !first_entry ||
        !entry_count || seen_sections[*raw_id] ||
        *first_entry > archive_entry_count ||
        *entry_count > archive_entry_count - *first_entry) {
      return std::nullopt;
    }

    seen_sections[*raw_id] = true;
    for (uint32_t entry = *first_entry; entry < *first_entry + *entry_count;
         ++entry) {
      if (seen_entries[entry]) {
        return std::nullopt;
      }
      seen_entries[entry] = true;
    }
    manifest.sections[*raw_id] = {
        .first_entry = *first_entry,
        .entry_count = *entry_count,
    };
  }

  if (reader.Remaining() != 0 ||
      !std::ranges::all_of(seen_sections, std::identity{}) ||
      !std::ranges::all_of(seen_entries, std::identity{})) {
    return std::nullopt;
  }
  return manifest;
}

std::vector<uint8_t> BuildDataManifest(
    const std::array<uint32_t, std::to_underlying(DataSectionId::Count)>
        &section_counts) {
  uint32_t first_entry = 1;
  for (const auto count : section_counts) {
    if (count > std::numeric_limits<uint32_t>::max() - first_entry) {
      return {};
    }
    first_entry += count;
  }

  util::ByteWriter writer;
  writer.WriteBytes(kMagic);
  writer.Write(kVersion);
  writer.Write(static_cast<uint32_t>(section_counts.size()));
  first_entry = 1;
  for (uint32_t id = 0; id < section_counts.size(); ++id) {
    writer.Write(id);
    writer.Write(first_entry);
    writer.Write(section_counts[id]);
    first_entry += section_counts[id];
  }
  return std::move(writer).TakeBytes();
}

} // namespace data
