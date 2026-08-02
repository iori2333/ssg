/// Runtime lookup for embedded localized text catalogs.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "localization.h"

#include "messages_data.h"
#include "util/byte_io.h"

namespace {

constexpr std::array kMusicTitleIds = {
    i18n::TextIdFromKey("music.track_00.title"),
    i18n::TextIdFromKey("music.track_01.title"),
    i18n::TextIdFromKey("music.track_02.title"),
    i18n::TextIdFromKey("music.track_03.title"),
    i18n::TextIdFromKey("music.track_04.title"),
    i18n::TextIdFromKey("music.track_05.title"),
    i18n::TextIdFromKey("music.track_06.title"),
    i18n::TextIdFromKey("music.track_07.title"),
    i18n::TextIdFromKey("music.track_08.title"),
    i18n::TextIdFromKey("music.track_09.title"),
    i18n::TextIdFromKey("music.track_10.title"),
    i18n::TextIdFromKey("music.track_11.title"),
    i18n::TextIdFromKey("music.track_12.title"),
    i18n::TextIdFromKey("music.track_13.title"),
    i18n::TextIdFromKey("music.track_14.title"),
    i18n::TextIdFromKey("music.track_15.title"),
    i18n::TextIdFromKey("music.track_16.title"),
    i18n::TextIdFromKey("music.track_17.title"),
    i18n::TextIdFromKey("music.track_18.title"),
    i18n::TextIdFromKey("music.track_19.title"),
};

constexpr std::array kMusicCommentIds = {
    i18n::TextIdFromKey("music.track_00.comment"),
    i18n::TextIdFromKey("music.track_01.comment"),
    i18n::TextIdFromKey("music.track_02.comment"),
    i18n::TextIdFromKey("music.track_03.comment"),
    i18n::TextIdFromKey("music.track_04.comment"),
    i18n::TextIdFromKey("music.track_05.comment"),
    i18n::TextIdFromKey("music.track_06.comment"),
    i18n::TextIdFromKey("music.track_07.comment"),
    i18n::TextIdFromKey("music.track_08.comment"),
    i18n::TextIdFromKey("music.track_09.comment"),
    i18n::TextIdFromKey("music.track_10.comment"),
    i18n::TextIdFromKey("music.track_11.comment"),
    i18n::TextIdFromKey("music.track_12.comment"),
    i18n::TextIdFromKey("music.track_13.comment"),
    i18n::TextIdFromKey("music.track_14.comment"),
    i18n::TextIdFromKey("music.track_15.comment"),
    i18n::TextIdFromKey("music.track_16.comment"),
    i18n::TextIdFromKey("music.track_17.comment"),
    i18n::TextIdFromKey("music.track_18.comment"),
    i18n::TextIdFromKey("music.track_19.comment"),
};

class CatalogReader {
public:
  explicit CatalogReader(std::span<const uint8_t> bytes) : reader_(bytes) {}

  [[nodiscard]] std::optional<uint32_t> ReadU32() {
    return reader_.Read<uint32_t>();
  }

  [[nodiscard]] std::optional<std::string_view> ReadString(size_t size) {
    const auto bytes = reader_.ReadBytes(size);
    if (!bytes) {
      return std::nullopt;
    }
    const auto *text = reinterpret_cast<const char *>(bytes->data());
    return std::string_view(text, bytes->size());
  }

  [[nodiscard]] bool ReadMagic(std::span<const uint8_t> magic) {
    const auto bytes = reader_.ReadBytes(magic.size());
    return bytes && std::ranges::equal(*bytes, magic);
  }

  [[nodiscard]] bool Empty() const { return reader_.Empty(); }

private:
  util::ByteReader reader_;
};

} // namespace

namespace i18n {

bool Localization::ParseCatalog(std::span<const uint8_t> bytes,
                                Catalog &catalog) {
  static constexpr std::array<uint8_t, 4> kMagic = {'S', 'S', 'T', 'X'};
  CatalogReader reader(bytes);
  if (!reader.ReadMagic(kMagic)) {
    return false;
  }
  const auto version = reader.ReadU32();
  const auto entry_count = reader.ReadU32();
  if (!version || *version != 2 || !entry_count) {
    return false;
  }

  catalog.messages.reserve(*entry_count);
  for (uint32_t entry_index = 0; entry_index < *entry_count; ++entry_index) {
    const auto id = reader.ReadU32();
    const auto size = reader.ReadU32();
    if (!id || !size) {
      return false;
    }
    const auto value = reader.ReadString(*size);
    if (!value) {
      return false;
    }

    std::vector<std::string_view> lines;
    size_t line_start = 0;
    while (line_start <= value->size()) {
      const auto line_end = value->find('\n', line_start);
      lines.push_back(value->substr(line_start, line_end - line_start));
      if (line_end == std::string_view::npos) {
        break;
      }
      line_start = line_end + 1;
    }
    if (!catalog.messages
             .emplace(*id,
                      TextEntry{.value = *value, .lines = std::move(lines)})
             .second) {
      return false;
    }
  }
  return reader.Empty();
}

bool Localization::Initialize(std::string_view requested_language) {
  std::vector<Catalog> catalogs;
  catalogs.reserve(embedded_message_catalog_count);
  for (size_t i = 0; i < embedded_message_catalog_count; ++i) {
    const auto &embedded = embedded_message_catalogs[i];
    Catalog catalog(embedded.language);
    if (!ParseCatalog({embedded.message_data, embedded.message_size},
                      catalog) ||
        !ParseCatalog({embedded.ui_data, embedded.ui_size}, catalog)) {
      return false;
    }
    if (!ParseCatalog({embedded.music_data, embedded.music_size}, catalog)) {
      return false;
    }
    catalogs.push_back(std::move(catalog));
  }

  const auto fallback = std::ranges::find(catalogs, "ja", &Catalog::language);
  if (fallback == catalogs.end()) {
    return false;
  }
  const auto fallback_index = static_cast<size_t>(fallback - catalogs.begin());
  const auto &fallback_messages = catalogs[fallback_index].messages;
  for (const auto &catalog : catalogs) {
    if (catalog.messages.size() != fallback_messages.size()) {
      return false;
    }
    for (const auto &[id, unused] : fallback_messages) {
      if (!catalog.messages.contains(id)) {
        return false;
      }
    }
  }
  catalogs_ = std::move(catalogs);
  fallback_ = fallback_index;
  current_ = fallback_;
  (void)SetLanguage(requested_language);
  return true;
}

bool Localization::SetLanguage(std::string_view language) {
  const auto found = std::ranges::find(catalogs_, language, &Catalog::language);
  if (found == catalogs_.end()) {
    return false;
  }
  current_ = static_cast<size_t>(found - catalogs_.begin());
  return true;
}

std::string_view Localization::Language() const { return LanguageAt(current_); }

std::string_view Localization::LanguageAt(size_t index) const {
  return index < catalogs_.size() ? catalogs_[index].language
                                  : std::string_view{};
}

bool Localization::HasText(size_t language_index, TextId id) const {
  return language_index < catalogs_.size() &&
         catalogs_[language_index].messages.contains(id);
}

std::string_view Localization::Text(TextId id) const {
  if (current_ < catalogs_.size()) {
    if (const auto found = catalogs_[current_].messages.find(id);
        found != catalogs_[current_].messages.end()) {
      return found->second.value;
    }
  }
  if (fallback_ < catalogs_.size()) {
    if (const auto found = catalogs_[fallback_].messages.find(id);
        found != catalogs_[fallback_].messages.end()) {
      return found->second.value;
    }
  }
  return {};
}

std::span<const std::string_view> Localization::Lines(TextId id) const {
  if (current_ < catalogs_.size()) {
    if (const auto found = catalogs_[current_].messages.find(id);
        found != catalogs_[current_].messages.end()) {
      return found->second.lines;
    }
  }
  if (fallback_ < catalogs_.size()) {
    if (const auto found = catalogs_[fallback_].messages.find(id);
        found != catalogs_[fallback_].messages.end()) {
      return found->second.lines;
    }
  }
  return {};
}

std::string_view Localization::MusicTitle(size_t track) const {
  return track < kMusicTitleIds.size() ? Text(kMusicTitleIds[track])
                                       : std::string_view{};
}

std::string_view Localization::MusicComment(size_t track) const {
  return track < kMusicCommentIds.size() ? Text(kMusicCommentIds[track])
                                         : std::string_view{};
}

} // namespace i18n
