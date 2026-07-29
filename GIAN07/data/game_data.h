///
/// GameData - owns validated game archives and their data-only catalogs
///
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "pbg_archive.h"

namespace data {

enum class ArchiveId : uint8_t {
  Map,
  Images,
  Music,
  Sound,
  Count,
};

enum class LoadErrorKind : uint8_t {
  Missing,
  Invalid,
};

struct LoadError {
  ArchiveId archive;
  LoadErrorKind kind;
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

  [[nodiscard]] size_t TrackCount() const { return music_tracks_.size(); }
  [[nodiscard]] std::string_view TrackTitle(size_t index) const;
  [[nodiscard]] std::string_view TrackComment(size_t index) const;

private:
  struct MusicTrack {
    std::string title;
    std::string comment;
    size_t midi_offset = 0;
  };

  [[nodiscard]] const PbgArchive &Archive(ArchiveId id) const;

  std::array<PbgArchive, std::to_underlying(ArchiveId::Count)> archives_;
  std::vector<MusicTrack> music_tracks_;
  bool loaded_ = false;
};

[[nodiscard]] std::string FormatLoadErrors(const LoadErrors &errors);

} // namespace data
