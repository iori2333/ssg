///
/// BitStream - bit-level buffer and file I/O
///

#include <cstddef>
#include <cstdint>

#include "bit_stream.h"
#include "file.h"

bool BitReader::CanRead(std::size_t bit_count) const {
  if (byte_cursor_ >= buffer_.size()) {
    return bit_count == 0;
  }
  return bit_count <= ((buffer_.size() - byte_cursor_) * 8 - bit_cursor_);
}

uint8_t BitReader::ReadBit() {
  if (!CanRead(1)) {
    return 0xff;
  }

  const auto bit =
      static_cast<uint8_t>((buffer_[byte_cursor_] >> (7 - bit_cursor_)) & 1);
  if (++bit_cursor_ == 8) {
    bit_cursor_ = 0;
    ++byte_cursor_;
  }
  return bit;
}

uint32_t BitReader::ReadBits(std::size_t bit_count) {
  if (bit_count == 0) {
    return 0;
  }
  if (bit_count > 24 || !CanRead(bit_count)) {
    return 0xffffffff;
  }

  const auto window_size = bit_cursor_ + bit_count;
  uint32_t window = static_cast<uint32_t>(buffer_[byte_cursor_]) << 24;
  if (bit_count > 1 && window_size > 8) {
    window |= static_cast<uint32_t>(buffer_[byte_cursor_ + 1]) << 16;
  }
  if (bit_count > 9 && window_size > 16) {
    window |= static_cast<uint32_t>(buffer_[byte_cursor_ + 2]) << 8;
  }
  if (bit_count > 17 && window_size > 24) {
    window |= buffer_[byte_cursor_ + 3];
  }

  window <<= bit_cursor_;
  const auto cursor = bit_cursor_ + bit_count;
  byte_cursor_ += cursor / 8;
  bit_cursor_ = static_cast<uint8_t>(cursor % 8);
  return window >> (32 - bit_count);
}

void BitWriter::WriteBit(uint8_t bit) {
  if (bit_cursor_ == 0) {
    buffer_.push_back(0);
  }
  buffer_.back() |= static_cast<uint8_t>((bit & 1) << (7 - bit_cursor_));
  bit_cursor_ = static_cast<uint8_t>((bit_cursor_ + 1) % 8);
}

void BitWriter::WriteBits(uint32_t bits, unsigned int bit_count) {
  if (bit_count == 0) {
    return;
  }
  uint32_t mask = 1U << (bit_count - 1);
  for (unsigned int i = 0; i < bit_count; ++i) {
    WriteBit(static_cast<uint8_t>((bits & mask) != 0));
    mask >>= 1;
  }
}

bool BitWriter::Save(const char *path) const { return SaveFile(path, buffer_); }

BitFileReader LoadBitFile(const char *path) {
  return BitFileReader{LoadFile(path)};
}
