///
/// GameData - owns the validated unified game data archive
///
#include <algorithm>
#include <array>
#include <charconv>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <utility>

#include "game_data.h"

#include "sys/file.h"

namespace {

constexpr std::string_view kDirectoryName = "data";
constexpr std::string_view kArchiveName = "data.pak";
constexpr std::array<uint32_t, std::to_underlying(data::DataSectionId::Count)>
    kMinimumEntries = {7, 39, 20, 20, 0};

bool IsStandardMidi(std::span<const uint8_t> data) {
  static constexpr std::array<uint8_t, 8> kHeader = {'M', 'T', 'h', 'd',
                                                     0,   0,   0,   6};
  return data.size() >= 14 &&
         std::ranges::equal(data.first(kHeader.size()), kHeader);
}

bool IsBitmap(std::span<const uint8_t> data) {
  return data.size() >= 14 && data[0] == 'B' && data[1] == 'M';
}

bool IsWave(std::span<const uint8_t> data) {
  return data.size() >= 12 && data[0] == 'R' && data[1] == 'I' &&
         data[2] == 'F' && data[3] == 'F' && data[8] == 'W' &&
         data[9] == 'A' && data[10] == 'V' && data[11] == 'E';
}

std::optional<uint32_t> ParseEntryIndex(const std::filesystem::path &path,
                                        std::string_view extension) {
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

std::optional<uint32_t>
CountDirectoryEntries(const std::filesystem::path &directory,
                      std::string_view extension) {
  std::error_code error;
  if (!std::filesystem::is_directory(directory, error)) {
    return std::nullopt;
  }

  std::vector<uint32_t> indices;
  for (std::filesystem::directory_iterator it(directory, error), end;
       !error && it != end; it.increment(error)) {
    if (!it->is_regular_file(error)) {
      continue;
    }
    const auto index = ParseEntryIndex(it->path(), extension);
    if (!index) {
      return std::nullopt;
    }
    indices.push_back(*index);
  }
  if (error) {
    return std::nullopt;
  }
  std::ranges::sort(indices);
  for (size_t i = 0; i < indices.size(); ++i) {
    if (indices[i] != i) {
      return std::nullopt;
    }
  }
  return static_cast<uint32_t>(indices.size());
}

} // namespace

namespace data {

LoadErrors GameData::Load(std::string_view data_path) {
  if (loaded_) {
    return {};
  }

  const auto root = std::filesystem::path(data_path);
  std::error_code error;
  const auto directory = root / kDirectoryName;
  if (std::filesystem::is_directory(directory, error)) {
    return LoadDirectory(directory);
  }
  return LoadArchive(root / kArchiveName);
}

LoadErrors GameData::LoadDirectory(const std::filesystem::path &path) {
  DataManifest manifest;
  for (size_t section = 0; section < manifest.sections.size(); ++section) {
    const auto count = CountDirectoryEntries(
        path / kDataSectionNames[section], kDataSectionExtensions[section]);
    if (!count || *count < kMinimumEntries[section] ||
        (section == std::to_underlying(DataSectionId::Demos) && *count > 6)) {
      return {{LoadErrorKind::Invalid, DataSourceKind::Directory}};
    }
    manifest.sections[section].entry_count = *count;
  }

  directory_ = path;
  manifest_ = manifest;
  loaded_ = true;
  if (!ValidateContents()) {
    directory_.clear();
    manifest_ = {};
    loaded_ = false;
    return {{LoadErrorKind::Invalid, DataSourceKind::Directory}};
  }
  return {};
}

LoadErrors GameData::LoadArchive(const std::filesystem::path &path) {
  std::error_code error;
  if (!std::filesystem::is_regular_file(path, error)) {
    return {{LoadErrorKind::Missing, DataSourceKind::Archive}};
  }

  auto archive = PbgArchive::Open(path);
  if (!archive || archive.EntryCount() == 0) {
    return {{LoadErrorKind::Invalid, DataSourceKind::Archive}};
  }

  const auto manifest =
      ParseDataManifest(archive.Extract(0), archive.EntryCount());
  if (!manifest) {
    return {{LoadErrorKind::Invalid, DataSourceKind::Archive}};
  }
  for (size_t i = 0; i < kMinimumEntries.size(); ++i) {
    if (manifest->sections[i].entry_count < kMinimumEntries[i]) {
      return {{LoadErrorKind::Invalid, DataSourceKind::Archive}};
    }
  }
  if (manifest->sections[std::to_underlying(DataSectionId::Demos)].entry_count >
      6) {
    return {{LoadErrorKind::Invalid, DataSourceKind::Archive}};
  }

  archive_ = std::move(archive);
  manifest_ = *manifest;
  loaded_ = true;
  if (!ValidateContents()) {
    archive_ = {};
    manifest_ = {};
    loaded_ = false;
    return {{LoadErrorKind::Invalid, DataSourceKind::Archive}};
  }
  return {};
}

bool GameData::ValidateContents() const {
  const auto validate_section = [this](DataSectionId section,
                                       const auto &validator) {
    const auto &range = manifest_.sections[std::to_underlying(section)];
    for (uint32_t index = 0; index < range.entry_count; ++index) {
      if (!validator(Extract(section, index))) {
        return false;
      }
    }
    return true;
  };

  if (!validate_section(DataSectionId::Maps,
                        [](const auto &bytes) { return !bytes.empty(); }) ||
      !validate_section(DataSectionId::Images, IsBitmap) ||
      !validate_section(DataSectionId::Music, IsStandardMidi) ||
      !validate_section(DataSectionId::Sounds, IsWave)) {
    return false;
  }

  const auto &demos =
      manifest_.sections[std::to_underlying(DataSectionId::Demos)];
  for (uint32_t index = 0; index < demos.entry_count; ++index) {
    const auto demo = PbgArchive::Open(Extract(DataSectionId::Demos, index));
    if (!demo || demo.EntryCount() < 2) {
      return false;
    }
  }
  return true;
}

std::vector<uint8_t> GameData::Extract(DataSectionId section,
                                       uint32_t index) const {
  const auto &range = manifest_.sections[std::to_underlying(section)];
  if (!loaded_ || index >= range.entry_count) {
    return {};
  }
  if (!directory_.empty()) {
    return File_Load(directory_ / kDataSectionNames[std::to_underlying(section)] /
                     std::format("{:03}{}", index,
                                 kDataSectionExtensions[std::to_underlying(
                                     section)]));
  }
  return archive_.Extract(range.first_entry + index);
}

std::vector<uint8_t> GameData::ExtractMap(uint32_t index) const {
  return Extract(DataSectionId::Maps, index);
}

std::vector<uint8_t> GameData::ExtractImage(uint32_t index) const {
  return Extract(DataSectionId::Images, index);
}

std::vector<uint8_t> GameData::ExtractSound(uint32_t index) const {
  return Extract(DataSectionId::Sounds, index);
}

std::vector<uint8_t> GameData::ExtractMusicMidi(uint32_t index) const {
  auto raw = Extract(DataSectionId::Music, index);
  return IsStandardMidi(raw) ? std::move(raw) : std::vector<uint8_t>{};
}

std::vector<uint8_t> GameData::ExtractDemo(uint32_t index) const {
  return Extract(DataSectionId::Demos, index);
}

std::string FormatLoadErrors(const LoadErrors &errors,
                             std::string_view missing_text,
                             std::string_view invalid_text) {
  std::string result;
  for (const auto &error : errors) {
    const auto source = error.source == DataSourceKind::Directory
                            ? kDirectoryName
                            : kArchiveName;
    result += std::format("{}: {}\n", source,
                          error.kind == LoadErrorKind::Missing ? missing_text
                                                               : invalid_text);
  }
  return result;
}

} // namespace data
