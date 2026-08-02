///
/// Little- and big-endian integer types
///
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace util {

template <typename T, bool BigEndian> class EndianValue {
  std::array<uint8_t, sizeof(T)> v{};

  using Unsigned = std::make_unsigned_t<T>;
  static constexpr Unsigned ShiftOffset(std::size_t byte) {
    return ((BigEndian ? (sizeof(T) - 1 - byte) : byte) * 8);
  }

public:
  EndianValue() noexcept = default;

  EndianValue(T other) noexcept { *this = other; }

  constexpr operator T() const noexcept {
    T ret = 0;
    for (std::size_t byte = 0; byte < sizeof(T); ++byte) {
      ret |= (static_cast<Unsigned>(v[byte]) << ShiftOffset(byte));
    }
    return ret;
  }

  EndianValue &operator=(T other) noexcept {
    for (std::size_t byte = 0; byte < sizeof(T); ++byte) {
      v[byte] = static_cast<uint8_t>(static_cast<Unsigned>(other) >>
                                     ShiftOffset(byte));
    }
    return *this;
  }
};

template <typename T> using LittleEndian = EndianValue<T, false>;
template <typename T> using BigEndian = EndianValue<T, true>;

} // namespace util
