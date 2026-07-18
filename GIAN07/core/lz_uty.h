///
/// LzUty - Packfiles and compression
///
#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "sys/file.h"
#include "util/endian.h"

// On-disk PBG entry descriptor. Exposed because PackFile references it, but
// not generally useful to external callers.
struct PbgFileInfo {
  ENDIAN_LITTLE<uint32_t> size_uncompressed;
  ENDIAN_LITTLE<uint32_t> offset;
  ENDIAN_LITTLE<uint32_t> checksum_compressed;
};

// Bit-level I/O (used for obfuscated high-score persistence and LZSS).
// Retained as-is — already cleanly class-based.
class BIT_DEVICE_READ {
  struct {
    size_t byte = 0;
    uint8_t bit = 0;

    void operator+=(unsigned int bitcount) {
      bit += bitcount;
      byte += (bit / 8);
      bit %= 8;
    }
  } cursor;

public:
  const BYTE_BUFFER_BORROWED buffer;

  BIT_DEVICE_READ(const BYTE_BUFFER_BORROWED buffer) : buffer(buffer) {}
  BIT_DEVICE_READ(const void *mem, size_t size)
      : buffer({static_cast<const uint8_t *>(mem), size}) {}

  // Returns 0xFF if we're at the end of the stream.
  uint8_t GetBit();
  // Returns 0xFFFFFFFF if we're at the end of the stream. Supports a maximum
  // of 24 bits.
  uint32_t GetBits(size_t bitcount);
};

struct BIT_DEVICE_WRITE {
  BYTE_BUFFER_GROWABLE buffer;
  uint8_t bit_cursor : 3 = 0;

  void PutBit(uint8_t bit);
  void PutBits(uint32_t bits, unsigned int bitcount);
  bool Write(const char *s) const;
};

struct BIT_FILE_READ : public BIT_DEVICE_READ {
  const BYTE_BUFFER_OWNED file;
  BIT_FILE_READ(BYTE_BUFFER_OWNED &&file)
      : BIT_DEVICE_READ(file.get(), file.size()), file(std::move(file)) {}
};

BIT_FILE_READ BitFilCreateR(const char *s);

// Packfile container (PBG format with LZSS compression).
// Provides transparent random access by entry index; entries are decompressed
// lazily on Extract().
class PackFile {
public:
  PackFile() = default;

  static PackFile Open(const char *path);
  static PackFile Open(SDL_IOStream &stream);

  // Decompresses entry [index] into a freshly allocated buffer.
  [[nodiscard]] BYTE_BUFFER_OWNED Extract(uint32_t index) const;

  uint32_t Count() const;
  explicit operator bool() const { return data_.get() != nullptr; }

private:
  BYTE_BUFFER_OWNED data_;
  std::span<const PbgFileInfo> entries_;

  friend PackFile OpenFromBuffer(BYTE_BUFFER_OWNED);
};

// Builder for writing PBG packfiles. Entries are added as raw byte spans and
// compressed with LZSS on Write().
class PackWriter {
public:
  void Add(std::span<const uint8_t> data);
  bool Write(const char *path,
             std::optional<FILE_TIMESTAMPS> timestamps = std::nullopt) const;

private:
  std::vector<BYTE_BUFFER_BORROWED> files_;
};