/// Runtime lookup for embedded localized text catalogs.

#include <algorithm>
#include <array>
#include <optional>
#include <utility>

#include "localization.h"

#include "messages_data.h"
#include "util/endian.h"

namespace {

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
    if (!ParseCatalog({embedded.data, embedded.size}, catalog)) {
      return false;
    }
    catalogs.push_back(std::move(catalog));
  }

  const auto fallback = std::ranges::find(catalogs, "ja", &Catalog::language);
  if (fallback == catalogs.end()) {
    return false;
  }
  const auto fallback_index = static_cast<size_t>(fallback - catalogs.begin());
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

} // namespace i18n
