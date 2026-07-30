///
/// GameData - owns validated game archives and their data-only catalogs
///
#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <optional>
#include <utility>

#include "game_data.h"

#include "util/byte_io.h"

namespace {

using data::ArchiveId;
using data::LoadErrorKind;

struct ArchiveSpec {
  ArchiveId id;
  std::string_view filename;
  uint32_t minimum_entries;
};

constexpr std::array kArchiveSpecs = {
    ArchiveSpec{ArchiveId::Map, "MAP.PAK", 13},
    ArchiveSpec{ArchiveId::Images, "IMAGES.PAK", 39},
    ArchiveSpec{ArchiveId::Music, "MUSIC.PAK", 20},
    ArchiveSpec{ArchiveId::Sound, "SOUND.PAK", 20},
};

constexpr size_t ToIndex(ArchiveId id) { return std::to_underlying(id); }

std::optional<data::PbgArchive> LoadArchive(std::string_view data_path,
                                            const ArchiveSpec &spec,
                                            data::LoadErrors &errors) {
  const auto path = std::string(data_path) + std::string(spec.filename);
  std::error_code error;
  if (!std::filesystem::is_regular_file(path, error)) {
    errors.push_back({spec.id, LoadErrorKind::Missing});
    return std::nullopt;
  }

  auto archive = data::PbgArchive::Open(path);
  if (!archive || archive.EntryCount() < spec.minimum_entries) {
    errors.push_back({spec.id, LoadErrorKind::Invalid});
    return std::nullopt;
  }
  return archive;
}

std::string_view ArchiveName(ArchiveId id) {
  const auto found = std::ranges::find(kArchiveSpecs, id, &ArchiveSpec::id);
  return found == kArchiveSpecs.end() ? std::string_view{} : found->filename;
}

} // namespace

namespace data {

LoadErrors GameData::Load(std::string_view data_path) {
  if (loaded_) {
    return {};
  }

  LoadErrors errors;
  std::array<PbgArchive, std::to_underlying(ArchiveId::Count)> archives;
  for (const auto &spec : kArchiveSpecs) {
    auto archive = LoadArchive(data_path, spec, errors);
    if (archive) {
      archives[ToIndex(spec.id)] = std::move(*archive);
    }
  }
  if (!errors.empty()) {
    return errors;
  }

  std::vector<MusicTrack> tracks;
  const auto &music = archives[ToIndex(ArchiveId::Music)];
  tracks.reserve(music.EntryCount());
  for (uint32_t index = 0; index < music.EntryCount(); ++index) {
    auto raw = music.Extract(index);
    if (raw.empty()) {
      errors.push_back({ArchiveId::Music, LoadErrorKind::Invalid});
      break;
    }

    util::ByteReader reader{raw};
    auto read_string = [&reader]() -> std::optional<std::string> {
      const auto length = reader.Read<uint32_t>();
      if (!length) {
        return std::nullopt;
      }
      const auto bytes = reader.ReadBytes(*length);
      if (!bytes) {
        return std::nullopt;
      }
      return std::string(reinterpret_cast<const char *>(bytes->data()),
                         bytes->size());
    };

    auto title = read_string();
    auto comment = read_string();
    if (!title || !comment || reader.Remaining() == 0) {
      errors.push_back({ArchiveId::Music, LoadErrorKind::Invalid});
      break;
    }
    tracks.push_back({std::move(*title), std::move(*comment),
                      raw.size() - reader.Remaining()});
  }
  if (!errors.empty()) {
    return errors;
  }

  archives_ = std::move(archives);
  music_tracks_ = std::move(tracks);
  loaded_ = true;
  return {};
}

const PbgArchive &GameData::Archive(ArchiveId id) const {
  return archives_[ToIndex(id)];
}

std::vector<uint8_t> GameData::ExtractMap(uint32_t index) const {
  return Archive(ArchiveId::Map).Extract(index);
}

std::vector<uint8_t> GameData::ExtractImage(uint32_t index) const {
  return Archive(ArchiveId::Images).Extract(index);
}

std::vector<uint8_t> GameData::ExtractSound(uint32_t index) const {
  return Archive(ArchiveId::Sound).Extract(index);
}

std::vector<uint8_t> GameData::ExtractMusicMidi(uint32_t index) const {
  if (index >= music_tracks_.size()) {
    return {};
  }
  auto raw = Archive(ArchiveId::Music).Extract(index);
  const auto offset = music_tracks_[index].midi_offset;
  if (raw.empty() || offset >= raw.size()) {
    return {};
  }

  return {raw.begin() + static_cast<ptrdiff_t>(offset), raw.end()};
}

std::string_view GameData::TrackTitle(size_t index) const {
  return index < music_tracks_.size() ? music_tracks_[index].title
                                      : std::string_view{};
}

std::string_view GameData::TrackComment(size_t index) const {
  return index < music_tracks_.size() ? music_tracks_[index].comment
                                      : std::string_view{};
}

std::string FormatLoadErrors(const LoadErrors &errors,
                             std::string_view missing_text,
                             std::string_view invalid_text) {
  std::string result;
  for (const auto &error : errors) {
    result += std::format("{}: {}\n", ArchiveName(error.archive),
                          error.kind == LoadErrorKind::Missing ? missing_text
                                                               : invalid_text);
  }
  return result;
}

} // namespace data
