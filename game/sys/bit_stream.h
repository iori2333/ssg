///
/// BitStream - bit-level buffer and file I/O
///
#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include "buffer.h"

class BitReader {
public:
  explicit BitReader(BYTE_BUFFER_BORROWED buffer) : buffer_(buffer) {}
  BitReader(const void *data, std::size_t size)
      : buffer_({static_cast<const uint8_t *>(data), size}) {}

  [[nodiscard]] bool CanRead(std::size_t bit_count) const;
  [[nodiscard]] uint8_t ReadBit();
  [[nodiscard]] uint32_t ReadBits(std::size_t bit_count);

private:
  BYTE_BUFFER_BORROWED buffer_;
  std::size_t byte_cursor_ = 0;
  uint8_t bit_cursor_ = 0;
};

class BitWriter {
public:
  void WriteBit(uint8_t bit);
  void WriteBits(uint32_t bits, unsigned int bit_count);
  [[nodiscard]] bool Save(const char *path) const;

  [[nodiscard]] const BYTE_BUFFER_GROWABLE &Buffer() const { return buffer_; }

private:
  BYTE_BUFFER_GROWABLE buffer_;
  uint8_t bit_cursor_ = 0;
};

class BitFileReader : public BitReader {
public:
  explicit BitFileReader(BYTE_BUFFER_OWNED file)
      : BitReader(file.get(), file.size()), file_(std::move(file)) {}

private:
  BYTE_BUFFER_OWNED file_;
};

[[nodiscard]] BitFileReader LoadBitFile(const char *path);
