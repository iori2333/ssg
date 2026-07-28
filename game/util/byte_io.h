/// Little-endian byte sequence reading and writing.

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace util {

class ByteWriter {
public:
  template <std::unsigned_integral T> void Write(T value) {
    for (std::size_t i = 0; i < sizeof(T); i++) {
      bytes_.push_back(static_cast<uint8_t>(value >> (i * 8)));
    }
  }

  void WriteBytes(std::span<const uint8_t> bytes) {
    bytes_.insert(bytes_.end(), bytes.begin(), bytes.end());
  }

  [[nodiscard]] const std::vector<uint8_t> &Bytes() const { return bytes_; }

private:
  std::vector<uint8_t> bytes_;
};

class ByteReader {
public:
  explicit ByteReader(std::span<const uint8_t> bytes) : bytes_(bytes) {}

  template <std::unsigned_integral T> [[nodiscard]] std::optional<T> Read() {
    if (bytes_.size() - position_ < sizeof(T)) {
      return std::nullopt;
    }
    T value = 0;
    for (std::size_t i = 0; i < sizeof(T); i++) {
      value |= static_cast<T>(bytes_[position_++]) << (i * 8);
    }
    return value;
  }

  [[nodiscard]] std::optional<std::span<const uint8_t>>
  ReadBytes(std::size_t size) {
    if (bytes_.size() - position_ < size) {
      return std::nullopt;
    }
    const auto result = bytes_.subspan(position_, size);
    position_ += size;
    return result;
  }

  [[nodiscard]] bool Empty() const { return position_ == bytes_.size(); }

private:
  std::span<const uint8_t> bytes_;
  std::size_t position_ = 0;
};

} // namespace util
