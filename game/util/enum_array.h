///
/// Enum-indexed std::array
///
#pragma once

#include <array>
#include <cassert>
#include <utility>

namespace util {

template <typename T, typename Id>
class EnumArray : public std::array<T, std::to_underlying(Id::Count)> {
  using Base = std::array<T, std::to_underlying(Id::Count)>;

public:
  // We could do something like
  //
  //	using T_ref_or_value = std::conditional_t<
  //		std::derived_from<T, std::string_view>, T, T&
  //	>;
  //
  // to opt into returning by value for certain classes, but this does not
  // compile on Visual Studio 2022 17.11.0 Preview 4.0.

  constexpr T &operator[](Id id) noexcept {
#pragma warning(suppress : 26445) // gsl.view
    return Base::operator[](std::to_underlying(id));
  }

  constexpr const T &operator[](Id id) const noexcept {
#pragma warning(suppress : 26445) // gsl.view
    return Base::operator[](std::to_underlying(id));
  }
};

namespace cast {

template <typename Id>
constexpr Id down_enum(std::underlying_type_t<Id> value) {
  assert(value < std::to_underlying(Id::Count));
  return static_cast<Id>(value);
}

} // namespace cast

} // namespace util
