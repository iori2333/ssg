///
/// PbgArchive - validated PBG packfile access
///
#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "sys/buffer.h"
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

  [[nodiscard]] static PbgArchive Open(const std::filesystem::path &path);

  // Decompresses entry [index] into a freshly allocated buffer.
  [[nodiscard]] BYTE_BUFFER_OWNED Extract(uint32_t index) const;

  [[nodiscard]] uint32_t EntryCount() const;
  explicit operator bool() const { return static_cast<bool>(data_); }

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
  [[nodiscard]] bool Write(const std::filesystem::path &path) const;

private:
  std::vector<std::vector<uint8_t>> files_;
};

} // namespace data
