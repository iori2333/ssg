///
/// GameData - owns the validated unified game data archive
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "data_manifest.h"
#include "pbg_archive.h"

namespace data {

enum class LoadErrorKind : uint8_t {
  Missing,
  Invalid,
};

enum class DataSourceKind : uint8_t {
  Directory,
  Archive,
};

struct LoadError {
  LoadErrorKind kind;
  DataSourceKind source;
};

using LoadErrors = std::vector<LoadError>;

class GameData {
public:
  [[nodiscard]] LoadErrors Load(std::string_view data_path);
  [[nodiscard]] bool Loaded() const { return loaded_; }

  [[nodiscard]] std::vector<uint8_t> ExtractMap(uint32_t index) const;
  [[nodiscard]] std::vector<uint8_t> ExtractImage(uint32_t index) const;
  [[nodiscard]] std::vector<uint8_t> ExtractSound(uint32_t index) const;
  [[nodiscard]] std::vector<uint8_t> ExtractMusicMidi(uint32_t index) const;
  [[nodiscard]] std::vector<uint8_t>
  ExtractArrangedMusicMidi(uint32_t index) const;
  [[nodiscard]] std::vector<uint8_t> ExtractDemo(uint32_t index) const;

  [[nodiscard]] size_t TrackCount() const {
    return manifest_.sections[std::to_underlying(DataSectionId::Music)]
        .entry_count;
  }
  [[nodiscard]] size_t DemoCount() const {
    return manifest_.sections[std::to_underlying(DataSectionId::Demos)]
        .entry_count;
  }

private:
  [[nodiscard]] LoadErrors LoadDirectory(const std::filesystem::path &path);
  [[nodiscard]] LoadErrors LoadArchive(const std::filesystem::path &path);
  [[nodiscard]] bool ValidateContents() const;
  [[nodiscard]] std::vector<uint8_t> Extract(DataSectionId section,
                                             uint32_t index) const;

  std::filesystem::path directory_;
  PbgArchive archive_;
  DataManifest manifest_;
  bool loaded_ = false;
};

[[nodiscard]] std::string FormatLoadErrors(const LoadErrors &errors,
                                           std::string_view missing_text,
                                           std::string_view invalid_text);

} // namespace data
