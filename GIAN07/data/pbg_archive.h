///
/// PbgArchive - validated PBG packfile access
///
#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

#include "util/endian.h"

namespace data {

// On-disk PBG entry descriptor.
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
  [[nodiscard]] static PbgArchive Open(std::span<const uint8_t> bytes);

  // Decompresses entry [index] into a freshly allocated buffer.
  [[nodiscard]] std::vector<uint8_t> Extract(uint32_t index) const;

  [[nodiscard]] uint32_t EntryCount() const;
  explicit operator bool() const { return !data_.empty(); }

private:
  std::vector<uint8_t> data_;
  std::vector<PbgEntryHeader> entries_;

  static PbgArchive FromBuffer(std::vector<uint8_t> data);
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
