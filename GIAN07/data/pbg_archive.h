///
/// PbgArchive - validated PBG packfile access
///
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "sys/file.h"
#include "util/endian.h"

namespace data {

// On-disk PBG entry descriptor. Exposed because PbgArchive references it, but
// not generally useful to external callers.
struct PbgEntryHeader {
  ENDIAN_LITTLE<uint32_t> size_uncompressed;
  ENDIAN_LITTLE<uint32_t> offset;
  ENDIAN_LITTLE<uint32_t> checksum_compressed;
};

// Packfile container (PBG format with LZSS compression).
// Provides transparent random access by entry index; entries are decompressed
// lazily on Extract().
class PbgArchive {
public:
  PbgArchive() = default;

  static PbgArchive Open(const char *path);
  // Takes ownership of stream and closes it after reading.
  static PbgArchive Open(SDL_IOStream *stream);

  // Decompresses entry [index] into a freshly allocated buffer.
  [[nodiscard]] BYTE_BUFFER_OWNED Extract(uint32_t index) const;

  uint32_t EntryCount() const;
  explicit operator bool() const { return data_.get() != nullptr; }

private:
  BYTE_BUFFER_OWNED data_;
  std::span<const PbgEntryHeader> entries_;

  static PbgArchive FromBuffer(BYTE_BUFFER_OWNED data);
};

// Builder for writing PBG packfiles. Add() copies each entry so the builder
// owns all input until it is compressed by Write().
class PbgArchiveWriter {
public:
  void Add(std::span<const uint8_t> data);
  bool Write(const char *path,
             std::optional<FILE_TIMESTAMPS> timestamps = std::nullopt) const;

private:
  std::vector<std::vector<uint8_t>> files_;
};

} // namespace data
