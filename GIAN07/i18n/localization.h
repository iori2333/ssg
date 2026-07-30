/// Runtime lookup for embedded localized text catalogs.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "util/text_id.h"

namespace i18n {

using TextId = uint32_t;

[[nodiscard]] constexpr TextId TextIdFromKey(std::string_view key) {
  return util::TextIdFromKey(key);
}

class Localization {
public:
  [[nodiscard]] bool Initialize(std::string_view requested_language);
  [[nodiscard]] bool SetLanguage(std::string_view language);

  [[nodiscard]] std::string_view Language() const;
  [[nodiscard]] size_t CurrentLanguageIndex() const { return current_; }
  [[nodiscard]] size_t LanguageCount() const { return catalogs_.size(); }
  [[nodiscard]] std::string_view LanguageAt(size_t index) const;
  [[nodiscard]] bool HasText(size_t language_index, TextId id) const;
  [[nodiscard]] std::string_view Text(TextId id) const;
  [[nodiscard]] std::span<const std::string_view> Lines(TextId id) const;
  [[nodiscard]] std::string_view MusicTitle(size_t track) const;
  [[nodiscard]] std::string_view MusicComment(size_t track) const;

private:
  struct Catalog {
    std::string_view language;
    std::unordered_map<TextId, std::vector<std::string_view>> messages;
  };

  [[nodiscard]] static bool ParseCatalog(std::span<const uint8_t> bytes,
                                         Catalog &catalog);

  std::vector<Catalog> catalogs_;
  size_t current_ = 0;
  size_t fallback_ = 0;
};

} // namespace i18n
