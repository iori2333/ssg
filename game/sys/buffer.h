///
/// Memory ownership semantics
///

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

struct BYTE_BUFFER_BORROWED : public std::span<const uint8_t> {
  using span::span;

  template <typename T, size_t N>
  BYTE_BUFFER_BORROWED(std::span<T, N> val)
      : span(reinterpret_cast<const uint8_t *>(val.data()), val.size_bytes()) {}
};

template <typename ConstOrNonConstByte>
struct BYTE_BUFFER_CURSOR : public std::span<ConstOrNonConstByte> {
  using std::span<ConstOrNonConstByte>::span;

  template <typename T>
  using transfer_const =
      std::conditional_t<std::is_const_v<ConstOrNonConstByte>, const T, T>;

  size_t cursor = 0;

  // Required to work around a C26495 false positive, for some reason?
  BYTE_BUFFER_CURSOR(const std::span<ConstOrNonConstByte> other)
      : std::span<ConstOrNonConstByte>(other) {}

  // Reads up to [n] contiguous values of type T from the active cursor
  // position if possible. If the function returns a valid span, all [n]
  // objects are safe to access.
  template <typename T>
  std::optional<std::span<transfer_const<T>>> next(size_t n = 1) {
    if (cursor > this->size() || n > (this->size() - cursor) / sizeof(T)) {
      return std::nullopt;
    }
    const auto cursor_new = cursor + (sizeof(T) * n);
#pragma warning(suppress : 26473) // type.1
    auto ret = std::span<transfer_const<T>>{
        reinterpret_cast<transfer_const<T> *>(this->data() + cursor), n};
    cursor = cursor_new;
    return ret;
  }
};

class BYTE_BUFFER_OWNED {
public:
  BYTE_BUFFER_OWNED(std::nullptr_t = nullptr) noexcept {}
  explicit BYTE_BUFFER_OWNED(size_t size) : data_(size) {}
  explicit BYTE_BUFFER_OWNED(std::vector<uint8_t> data)
      : data_(std::move(data)) {}

  [[nodiscard]] explicit operator bool() const { return !data_.empty(); }
  [[nodiscard]] uint8_t *get() { return data_.data(); }
  [[nodiscard]] const uint8_t *get() const { return data_.data(); }
  [[nodiscard]] size_t size() const { return data_.size(); }

  // Borrows a buffer with an immutable cursor.
  BYTE_BUFFER_CURSOR<const uint8_t> cursor() const { return {get(), size()}; }

  // Borrows a buffer with a mutable cursor.
  BYTE_BUFFER_CURSOR<uint8_t> cursor_mut() { return {get(), size()}; }

private:
  std::vector<uint8_t> data_;
};

using BYTE_BUFFER_GROWABLE = std::vector<uint8_t>;
