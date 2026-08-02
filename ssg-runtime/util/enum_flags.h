///
/// Enum flag operators
///
#pragma once

#include <bit>
#include <type_traits>
#include <utility>

namespace util {

template <typename T> inline constexpr bool EnableEnumFlags = false;

} // namespace util

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr bool operator!(const T &v) noexcept {
  return (std::to_underlying(v) == 0);
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr std::underlying_type_t<T> operator~(const T &v) noexcept {
  return ~std::to_underlying(v);
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T operator&(const T &a, const T &b) noexcept {
  return static_cast<T>(std::to_underlying(a) & std::to_underlying(b));
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T operator|(const T &a, const T &b) noexcept {
  return static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T operator^(const T &a, const T &b) noexcept {
  return static_cast<T>(std::to_underlying(a) ^ std::to_underlying(b));
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T &operator|=(T &a, const T &b) noexcept {
  a = static_cast<T>(std::to_underlying(a) | std::to_underlying(b));
  return a;
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T &operator&=(T &a, const std::underlying_type_t<T> &b) noexcept {
  a = static_cast<T>(std::to_underlying(a) & b);
  return a;
}

template <typename T>
  requires util::EnableEnumFlags<T>
constexpr T &operator^=(T &a, const T &b) noexcept {
  a = static_cast<T>(a ^ b);
  return a;
}

// Sets the given [flag] in [self] to the value of [val].
template <typename T>
  requires util::EnableEnumFlags<T>
constexpr void SetEnumFlag(T &self, T flag, std::underlying_type_t<T> val) {
  const auto shift = std::countr_zero(std::to_underlying(flag));
  self = static_cast<T>(std::to_underlying(self) & ~flag);
  self |= static_cast<T>(static_cast<T>(val << shift) & flag);
}
