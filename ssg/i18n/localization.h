/// Runtime lookup for embedded localized text catalogs.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>
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
  struct TextEntry {
    std::string_view value;
    std::vector<std::string_view> lines;
  };

  struct Catalog {
    Catalog() = default;
    explicit Catalog(std::string_view language) : language(language) {}
    Catalog(const Catalog &) = default;
    Catalog &operator=(const Catalog &) = default;
    Catalog(Catalog &&other) noexcept : language(other.language) {
      try {
        messages = std::move(other.messages);
      } catch (...) {
        messages.clear();
      }
    }
    Catalog &operator=(Catalog &&other) noexcept {
      try {
        messages = std::move(other.messages);
      } catch (...) {
        messages.clear();
      }
      language = other.language;
      return *this;
    }
    ~Catalog() noexcept = default;

    std::string_view language;
    std::unordered_map<TextId, TextEntry> messages;
  };

  [[nodiscard]] static bool ParseCatalog(std::span<const uint8_t> bytes,
                                         Catalog &catalog);

  std::vector<Catalog> catalogs_;
  size_t current_ = 0;
  size_t fallback_ = 0;
};

} // namespace i18n
