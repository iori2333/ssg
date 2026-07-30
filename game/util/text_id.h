/// Stable text-key hashing shared by runtime lookup and catalog tools.
#pragma once

#include <cstdint>
#include <string_view>

namespace util {

[[nodiscard]] constexpr uint32_t TextIdFromKey(std::string_view key) {
  uint32_t hash = 2166136261U;
  for (const unsigned char c : key) {
    hash = (hash ^ c) * 16777619U;
  }
  return hash;
}

} // namespace util
