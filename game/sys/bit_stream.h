///
/// BitStream - bit-level buffer and file I/O
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

class BitReader {
public:
  explicit BitReader(std::span<const uint8_t> buffer) : buffer_(buffer) {}
  BitReader(const void *data, std::size_t size)
      : buffer_({static_cast<const uint8_t *>(data), size}) {}

  [[nodiscard]] bool CanRead(std::size_t bit_count) const;
  [[nodiscard]] uint8_t ReadBit();
  [[nodiscard]] uint32_t ReadBits(std::size_t bit_count);

private:
  std::span<const uint8_t> buffer_;
  std::size_t byte_cursor_ = 0;
  uint8_t bit_cursor_ = 0;
};

class BitWriter {
public:
  void WriteBit(uint8_t bit);
  void WriteBits(uint32_t bits, unsigned int bit_count);
  [[nodiscard]] bool Save(const char *path) const;

  [[nodiscard]] const std::vector<uint8_t> &Buffer() const { return buffer_; }

private:
  std::vector<uint8_t> buffer_;
  uint8_t bit_cursor_ = 0;
};

class BitFileReader : public BitReader {
public:
  explicit BitFileReader(std::vector<uint8_t> file)
      : BitReader(file.data(), file.size()), file_(std::move(file)) {}

private:
  std::vector<uint8_t> file_;
};

[[nodiscard]] BitFileReader LoadBitFile(const char *path);
