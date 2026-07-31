///
/// pack_tool - extract and repack PBG archives
///
/// Usage:
///   pack_tool extract <packfile> <out_dir>
///   pack_tool pack <in_dir> <packfile>
///

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "data/data_manifest.h"
#include "data/pbg_archive.h"
#include "sys/input.h"

namespace fs = std::filesystem;

namespace {

using EntryList = std::vector<std::vector<uint8_t>>;

constexpr auto kSectionCount = std::to_underlying(data::DataSectionId::Count);

void PrintUsage() {
  std::println(stderr, "Usage:");
  std::println(stderr, "  pack_tool extract <packfile> <out_dir>");
  std::println(stderr, "  pack_tool pack <in_dir> <packfile>");
}

std::optional<std::vector<uint8_t>> ReadFile(const fs::path &path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream) {
    return std::nullopt;
  }
  const auto end = stream.tellg();
  if (end < 0) {
    return std::nullopt;
  }
  std::vector<uint8_t> bytes(static_cast<size_t>(end));
  stream.seekg(0);
  if (!bytes.empty()) {
    stream.read(reinterpret_cast<char *>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
  }
  if (!stream) {
    return std::nullopt;
  }
  return bytes;
}

bool WriteFile(const fs::path &path, std::span<const uint8_t> bytes) {
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream) {
    return false;
  }
  if (!bytes.empty()) {
    stream.write(reinterpret_cast<const char *>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
  }
  return static_cast<bool>(stream);
}

std::optional<uint32_t> ParseEntryIndex(const fs::path &path,
                                        std::string_view extension) {
  if (path.extension() != extension) {
    return std::nullopt;
  }
  const auto stem = path.stem().string();
  if (stem.empty()) {
    return std::nullopt;
  }
  uint32_t index = 0;
  const auto [end, error] =
      std::from_chars(stem.data(), stem.data() + stem.size(), index);
  if (error != std::errc{} || end != stem.data() + stem.size()) {
    return std::nullopt;
  }
  return index;
}

std::optional<EntryList> ReadEntryDirectory(const fs::path &directory,
                                            std::string_view extension,
                                            bool allow_empty = false) {
  std::error_code error;
  if (!fs::is_directory(directory, error)) {
    std::println(stderr, "Not a directory: {}", directory.string());
    return std::nullopt;
  }

  std::vector<std::pair<uint32_t, fs::path>> paths;
  for (fs::directory_iterator it(directory, error), end; !error && it != end;
       it.increment(error)) {
    if (!it->is_regular_file(error) || it->path().extension() != extension) {
      continue;
    }
    const auto index = ParseEntryIndex(it->path(), extension);
    if (!index) {
      std::println(stderr, "Invalid entry filename: {}",
                   it->path().filename().string());
      return std::nullopt;
    }
    paths.emplace_back(*index, it->path());
  }
  if (error) {
    std::println(stderr, "Could not enumerate {}: {}", directory.string(),
                 error.message());
    return std::nullopt;
  }
  std::ranges::sort(paths, {}, &std::pair<uint32_t, fs::path>::first);
  if (paths.empty() && !allow_empty) {
    std::println(stderr, "No numeric {} entries found in {}", extension,
                 directory.string());
    return std::nullopt;
  }

  EntryList entries;
  entries.reserve(paths.size());
  for (size_t i = 0; i < paths.size(); ++i) {
    if (paths[i].first != i) {
      std::println(stderr,
                   "Entries in {} must be numbered contiguously from 000{}",
                   directory.string(), extension);
      return std::nullopt;
    }
    auto bytes = ReadFile(paths[i].second);
    if (!bytes) {
      std::println(stderr, "Could not read {}", paths[i].second.string());
      return std::nullopt;
    }
    entries.push_back(std::move(*bytes));
  }
  return entries;
}

bool IsStandardMidi(std::span<const uint8_t> bytes) {
  static constexpr std::array<uint8_t, 8> kHeader = {'M', 'T', 'h', 'd',
                                                     0,   0,   0,   6};
  return bytes.size() >= 14 &&
         std::ranges::equal(bytes.first(kHeader.size()), kHeader);
}

bool IsBitmap(std::span<const uint8_t> bytes) {
  return bytes.size() >= 14 && bytes[0] == 'B' && bytes[1] == 'M';
}

bool IsWave(std::span<const uint8_t> bytes) {
  return bytes.size() >= 12 && bytes[0] == 'R' && bytes[1] == 'I' &&
         bytes[2] == 'F' && bytes[3] == 'F' && bytes[8] == 'W' &&
         bytes[9] == 'A' && bytes[10] == 'V' && bytes[11] == 'E';
}

std::optional<uint32_t> ReadU32(std::span<const uint8_t> bytes, size_t offset) {
  if (offset > bytes.size() || bytes.size() - offset < 4) {
    return std::nullopt;
  }
  return static_cast<uint32_t>(bytes[offset]) |
         (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

std::optional<std::vector<uint8_t>>
NormalizeMusicEntry(std::span<const uint8_t> bytes) {
  if (IsStandardMidi(bytes)) {
    return std::vector<uint8_t>(bytes.begin(), bytes.end());
  }

  const auto title_size = ReadU32(bytes, 0);
  if (!title_size || *title_size > bytes.size() - 4) {
    return std::nullopt;
  }
  const size_t comment_size_offset = 4 + *title_size;
  const auto comment_size = ReadU32(bytes, comment_size_offset);
  if (!comment_size || *comment_size > bytes.size() - comment_size_offset - 4) {
    return std::nullopt;
  }
  const size_t midi_offset = comment_size_offset + 4 + *comment_size;
  const auto midi = bytes.subspan(midi_offset);
  if (!IsStandardMidi(midi)) {
    return std::nullopt;
  }
  return std::vector<uint8_t>(midi.begin(), midi.end());
}

void NormalizeLegacyMusic(EntryList &entries) {
  EntryList normalized;
  normalized.reserve(entries.size());
  for (const auto &entry : entries) {
    auto midi = NormalizeMusicEntry(entry);
    if (!midi) {
      return;
    }
    normalized.push_back(std::move(*midi));
  }
  entries = std::move(normalized);
}

bool WriteEntries(const fs::path &directory, const EntryList &entries,
                  std::string_view extension = ".bin") {
  std::error_code error;
  fs::create_directories(directory, error);
  if (error) {
    std::println(stderr, "Could not create {}: {}", directory.string(),
                 error.message());
    return false;
  }
  for (size_t i = 0; i < entries.size(); ++i) {
    const auto path = directory / std::format("{:03}{}", i, extension);
    if (!WriteFile(path, entries[i])) {
      std::println(stderr, "Could not write {}", path.string());
      return false;
    }
  }
  return true;
}

int ExtractArchive(const fs::path &packfile, const fs::path &output) {
  const auto archive = data::PbgArchive::Open(packfile);
  if (!archive) {
    std::println(stderr, "Invalid PBG archive: {}", packfile.string());
    return 1;
  }

  if (archive.EntryCount() > 0) {
    const auto manifest =
        data::ParseDataManifest(archive.Extract(0), archive.EntryCount());
    if (manifest) {
      for (size_t section = 0; section < kSectionCount; ++section) {
        const auto &range = manifest->sections[section];
        EntryList entries;
        entries.reserve(range.entry_count);
        for (uint32_t i = 0; i < range.entry_count; ++i) {
          entries.push_back(archive.Extract(range.first_entry + i));
        }
        if (!WriteEntries(output / data::kDataSectionNames[section], entries,
                          data::kDataSectionExtensions[section])) {
          return 1;
        }
      }
      std::println("Extracted data.pak: {} resource entries",
                   archive.EntryCount() - 1);
      return 0;
    }
  }

  EntryList entries;
  entries.reserve(archive.EntryCount());
  for (uint32_t i = 0; i < archive.EntryCount(); ++i) {
    entries.push_back(archive.Extract(i));
  }
  NormalizeLegacyMusic(entries);
  if (!WriteEntries(output, entries)) {
    return 1;
  }
  std::println("Extracted {} entries", entries.size());
  return 0;
}

bool WriteArchive(const fs::path &packfile, const EntryList &entries,
                  std::span<const uint8_t> manifest = {}) {
  data::PbgArchiveWriter writer;
  if (!manifest.empty()) {
    writer.Add(manifest);
  }
  for (const auto &entry : entries) {
    writer.Add(entry);
  }

  std::error_code error;
  const auto parent = packfile.parent_path();
  if (!parent.empty()) {
    fs::create_directories(parent, error);
  }
  if (error || !writer.Write(packfile)) {
    std::println(stderr, "Could not write {}", packfile.string());
    return false;
  }
  return true;
}

int PackUnified(const fs::path &input, const fs::path &packfile) {
  std::array<uint32_t, kSectionCount> section_counts{};
  EntryList combined;
  for (size_t section = 0; section < kSectionCount; ++section) {
    const bool is_demo =
        section == std::to_underlying(data::DataSectionId::Demos);
    auto entries =
        ReadEntryDirectory(input / data::kDataSectionNames[section],
                           data::kDataSectionExtensions[section], is_demo);
    if (!entries) {
      return 1;
    }
    const auto contents_valid = [&] {
      switch (static_cast<data::DataSectionId>(section)) {
      case data::DataSectionId::Maps:
        return std::ranges::all_of(
            *entries, [](const auto &entry) { return !entry.empty(); });
      case data::DataSectionId::Images:
        return std::ranges::all_of(*entries, IsBitmap);
      case data::DataSectionId::Music:
      case data::DataSectionId::MusicArranged:
        return std::ranges::all_of(*entries, IsStandardMidi);
      case data::DataSectionId::Sounds:
        return std::ranges::all_of(*entries, IsWave);
      case data::DataSectionId::Demos:
        return true;
      case data::DataSectionId::Count:
        return false;
      }
      return false;
    }();
    if (!contents_valid) {
      std::println(stderr, "Invalid file contents in the {} section",
                   data::kDataSectionNames[section]);
      return 1;
    }
    if (is_demo && entries->size() > 6) {
      std::println(stderr, "The demos section can contain at most 6 entries");
      return 1;
    }
    if (is_demo) {
      for (size_t i = 0; i < entries->size(); ++i) {
        const auto replay = data::PbgArchive::Open((*entries)[i]);
        const auto manifest =
            replay ? replay.Extract(0) : EntryList::value_type{};
        const auto inputs = replay.EntryCount() == 2 ? replay.Extract(1)
                                                     : EntryList::value_type{};
        const size_t stage_count_offset =
            manifest.size() >= 20 ? 20 + manifest[19] : manifest.size();
        bool has_demo_start = false;
        for (size_t offset = 0; offset + 1 < inputs.size(); offset += 2) {
          const auto input = static_cast<uint16_t>(inputs[offset]) |
                             (static_cast<uint16_t>(inputs[offset + 1]) << 8);
          has_demo_start |= (input & KEY_DEMO_START) != 0;
        }
        const bool valid =
            replay.EntryCount() == 2 && manifest.size() >= 6 &&
            manifest[0] == 'G' && manifest[1] == '7' && manifest[2] == 'R' &&
            manifest[3] == 'P' && manifest[4] == 2 && manifest[5] == 0 &&
            stage_count_offset + 1 < manifest.size() &&
            manifest[stage_count_offset] == 1 &&
            manifest[stage_count_offset + 1] == i &&
            inputs.size() % sizeof(uint16_t) == 0 && has_demo_start;
        if (!valid) {
          std::println(stderr,
                       "Demo {:03}.dat must be a single-stage Replay v2 for "
                       "Stage {}",
                       i, i + 1);
          return 1;
        }
      }
    }
    section_counts[section] = static_cast<uint32_t>(entries->size());
    combined.insert(combined.end(), std::make_move_iterator(entries->begin()),
                    std::make_move_iterator(entries->end()));
  }
  if (section_counts[std::to_underlying(data::DataSectionId::Music)] !=
      section_counts[std::to_underlying(data::DataSectionId::MusicArranged)]) {
    std::println(stderr,
                 "The music and music-arranged sections must contain the "
                 "same number of entries");
    return 1;
  }

  const auto manifest = data::BuildDataManifest(section_counts);
  if (manifest.empty() || !WriteArchive(packfile, combined, manifest)) {
    return 1;
  }
  std::println("Packed data.pak: {} resource entries, {} archive entries",
               combined.size(), combined.size() + 1);
  return 0;
}

int PackArchive(const fs::path &input, const fs::path &packfile) {
  const auto has_section = [&](std::string_view name) {
    std::error_code error;
    return fs::is_directory(input / name, error);
  };
  const bool is_unified =
      std::ranges::all_of(data::kDataSectionNames, has_section);
  if (is_unified) {
    return PackUnified(input, packfile);
  }

  auto entries = ReadEntryDirectory(input, ".bin");
  if (!entries || !WriteArchive(packfile, *entries)) {
    return 1;
  }
  std::println("Packed {} entries", entries->size());
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 4) {
    PrintUsage();
    return 1;
  }
  const std::string_view command = argv[1];
  if (command == "extract") {
    return ExtractArchive(argv[2], argv[3]);
  }
  if (command == "pack") {
    return PackArchive(argv[2], argv[3]);
  }
  PrintUsage();
  return 1;
}
