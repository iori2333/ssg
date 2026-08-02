///
/// Enum-indexed std::array
///
#pragma once

#include <array>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace util {

template <typename Id>
concept EnumArrayId =
    std::is_enum_v<Id> && std::is_unsigned_v<std::underlying_type_t<Id>> &&
    requires { Id::Count; };

template <typename T, typename Id>
  requires EnumArrayId<Id>
class EnumArray {
  using Array = std::array<T, std::to_underlying(Id::Count)>;

public:
  using size_type = typename Array::size_type;
  using iterator = typename Array::iterator;
  using const_iterator = typename Array::const_iterator;

  // We could do something like
  //
  //	using T_ref_or_value = std::conditional_t<
  //		std::derived_from<T, std::string_view>, T, T&
  //	>;
  //
  // to opt into returning by value for certain classes, but this does not
  // compile on Visual Studio 2022 17.11.0 Preview 4.0.

  constexpr EnumArray() = default;

  constexpr EnumArray(std::initializer_list<T> values) : data_{} {
    if (values.size() != data_.size()) {
      throw std::invalid_argument("EnumArray initializer size mismatch");
    }
    auto out = data_.begin();
    for (const auto &value : values) {
      *out = value;
      ++out;
    }
  }

  constexpr T &operator[](Id id) noexcept {
#pragma warning(suppress : 26445) // gsl.view
    return data_[std::to_underlying(id)];
  }

  constexpr const T &operator[](Id id) const noexcept {
#pragma warning(suppress : 26445) // gsl.view
    return data_[std::to_underlying(id)];
  }

  constexpr iterator begin() noexcept { return data_.begin(); }
  constexpr const_iterator begin() const noexcept { return data_.begin(); }
  constexpr iterator end() noexcept { return data_.end(); }
  constexpr const_iterator end() const noexcept { return data_.end(); }

  [[nodiscard]] constexpr size_type size() const noexcept {
    return data_.size();
  }

private:
  Array data_{};
};

} // namespace util
