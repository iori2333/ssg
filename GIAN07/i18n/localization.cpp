/// Runtime lookup for embedded localized text catalogs.

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "localization.h"

#include "messages_data.h"
#include "util/endian.h"

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
  explicit CatalogReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

  [[nodiscard]] std::optional<uint16_t> ReadU16() {
    if (bytes_.size() - offset_ < sizeof(uint16_t)) {
      return std::nullopt;
    }
    const auto value = static_cast<uint16_t>(U16LEAt(bytes_.data() + offset_));
    offset_ += sizeof(uint16_t);
    return value;
  }

  [[nodiscard]] std::optional<uint32_t> ReadU32() {
    if (bytes_.size() - offset_ < sizeof(uint32_t)) {
      return std::nullopt;
    }
    const auto value = static_cast<uint32_t>(U32LEAt(bytes_.data() + offset_));
    offset_ += sizeof(uint32_t);
    return value;
  }

  [[nodiscard]] std::optional<std::string_view> ReadString(size_t size) {
    if (bytes_.size() - offset_ < size) {
      return std::nullopt;
    }
    const auto *text = reinterpret_cast<const char *>(bytes_.data() + offset_);
    offset_ += size;
    return std::string_view(text, size);
  }

  [[nodiscard]] bool ReadMagic(std::span<const uint8_t> magic) {
    if (bytes_.size() - offset_ < magic.size() ||
        !std::ranges::equal(bytes_.subspan(offset_, magic.size()), magic)) {
      return false;
    }
    offset_ += magic.size();
    return true;
  }

  [[nodiscard]] bool Empty() const { return offset_ == bytes_.size(); }

private:
  std::span<const uint8_t> bytes_;
  size_t offset_ = 0;
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
  if (!version || *version != 1 || !entry_count) {
    return false;
  }

  catalog.messages.reserve(*entry_count);
  for (uint32_t entry_index = 0; entry_index < *entry_count; ++entry_index) {
    const auto id = reader.ReadU32();
    const auto line_count = reader.ReadU16();
    if (!id || !line_count || *line_count == 0) {
      return false;
    }
    std::vector<std::string_view> lines;
    lines.reserve(*line_count);
    for (uint16_t line_index = 0; line_index < *line_count; ++line_index) {
      const auto size = reader.ReadU32();
      if (!size) {
        return false;
      }
      const auto line = reader.ReadString(*size);
      if (!line) {
        return false;
      }
      lines.push_back(*line);
    }
    if (!catalog.messages.emplace(*id, std::move(lines)).second) {
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
    Catalog catalog{.language = embedded.language};
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
  const auto lines = Lines(id);
  return lines.empty() ? std::string_view{} : lines.front();
}

std::span<const std::string_view> Localization::Lines(TextId id) const {
  if (current_ < catalogs_.size()) {
    if (const auto found = catalogs_[current_].messages.find(id);
        found != catalogs_[current_].messages.end()) {
      return found->second;
    }
  }
  if (fallback_ < catalogs_.size()) {
    if (const auto found = catalogs_[fallback_].messages.find(id);
        found != catalogs_[fallback_].messages.end()) {
      return found->second;
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
