///
/// Directional casts for integers
///
#pragma once

#include <type_traits>
#include <utility>

namespace Cast {

template <typename T, typename F> constexpr T down(F &&f) noexcept {
  using To = std::remove_reference_t<T>;
  using From = std::remove_reference_t<F>;
  static_assert(sizeof(To) < sizeof(From));
  static_assert(std::is_signed_v<To> == std::is_signed_v<From>);
  return static_cast<T>(std::forward<F>(f));
}

template <typename T, typename F> constexpr T sign(F &&f) noexcept {
  using To = std::remove_reference_t<T>;
  using From = std::remove_reference_t<F>;
  static_assert(sizeof(To) == sizeof(From));
  static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
  return static_cast<T>(std::forward<F>(f));
}

template <typename T, typename F> constexpr T down_sign(F &&f) noexcept {
  using To = std::remove_reference_t<T>;
  using From = std::remove_reference_t<F>;
  static_assert(sizeof(To) < sizeof(From));
  static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
  return static_cast<T>(std::forward<F>(f));
}

template <typename T, typename F> constexpr T up(F &&f) noexcept {
  using To = std::remove_reference_t<T>;
  using From = std::remove_reference_t<F>;
  static_assert(sizeof(To) > sizeof(From));
  static_assert(std::is_signed_v<To> == std::is_signed_v<From>);
  return static_cast<T>(std::forward<F>(f));
}

template <typename T, typename F> constexpr T up_sign(F &&f) noexcept {
  using To = std::remove_reference_t<T>;
  using From = std::remove_reference_t<F>;
  static_assert(sizeof(To) > sizeof(From));
  static_assert(std::is_signed_v<To> != std::is_signed_v<From>);
  return static_cast<T>(std::forward<F>(f));
}

} // namespace Cast
