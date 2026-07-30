///
/// PbgArchive - validated PBG packfile access
///
#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include "pbg_archive.h"

#include "sys/bit_stream.h"
#include "sys/file.h"
#include "util/byte_io.h"

namespace data {

namespace {

constexpr auto LZSS_DICT_BITS = 13;
constexpr auto LZSS_SEQ_BITS = 4;
constexpr auto LZSS_SEQ_MIN = 3;
constexpr auto LZSS_DICT_MASK = ((1 << LZSS_DICT_BITS) - 1);
constexpr auto LZSS_SEQ_MAX = (LZSS_SEQ_MIN + ((1 << LZSS_SEQ_BITS) - 1));

constexpr const std::array<char, 4> kPbgHeadName = {'P', 'B', 'G', 0x1A};

struct PbgHeader {
  std::array<char, kPbgHeadName.size()> name = kPbgHeadName;
  ENDIAN_LITTLE<uint32_t> sum = 0;
  ENDIAN_LITTLE<uint32_t> n = 0;
};

} // namespace

// ---- LZSS -----------------------------------------------------------

template <typename Container>
uint32_t AccumulateEntryChecksum(uint32_t &current_total, uint32_t offset,
                                 uint32_t size_uncompressed,
                                 const Container &compressed) {
  auto ret = std::accumulate(compressed.begin(), compressed.end(), uint32_t{0});
  current_total += ret;
  current_total += size_uncompressed;
  current_total += offset;
  return ret;
}

std::optional<std::span<const uint8_t>>
CompressedEntry(std::span<const uint8_t> packfile,
                std::span<const PbgEntryHeader> info, uint32_t filno) {
  if (filno >= info.size()) {
    return std::nullopt;
  }
  const size_t start = info[filno].offset;
  const auto end =
      ((filno == (info.size() - 1)) ? packfile.size()
                                    : size_t{info[filno + 1].offset});
  if ((start >= packfile.size()) || (end < start) || (end > packfile.size())) {
    return std::nullopt;
  }
  return packfile.subspan(start, end - start);
}

static std::vector<uint8_t> Decompress(std::span<const uint8_t> compressed,
                                       uint32_t size_uncompressed) {
  std::vector<uint8_t> uncompressed(size_uncompressed);

  std::array<uint8_t, (1 << LZSS_DICT_BITS)> dict{};
  uint32_t out_i = 0;

  auto output = [&](uint8_t literal) {
    uncompressed[out_i] = literal;
    dict[out_i & LZSS_DICT_MASK] = literal;
    out_i++;
  };

  BitReader device{compressed};
  while (out_i < size_uncompressed) {
    if (!device.CanRead(1)) {
      return {};
    }
    const bool is_literal = device.ReadBit() != 0U;
    if (is_literal) {
      if (!device.CanRead(8)) {
        return {};
      }
      output(device.ReadBits(8));
    } else {
      if (!device.CanRead(LZSS_DICT_BITS)) {
        return {};
      }
      auto seq_offset = device.ReadBits(LZSS_DICT_BITS);
      if (seq_offset == 0) {
        return {};
      }
      seq_offset--;
      if (!device.CanRead(LZSS_SEQ_BITS)) {
        return {};
      }
      const auto seq_length = (device.ReadBits(LZSS_SEQ_BITS) + LZSS_SEQ_MIN);
      if (seq_length > size_uncompressed - out_i) {
        return {};
      }
      for (auto i = decltype(seq_length){0}; i < seq_length; i++) {
        output(dict[seq_offset++ & LZSS_DICT_MASK]);
      }
    }
  }
  return uncompressed;
}

std::vector<uint8_t> Compress(std::span<const uint8_t> buffer) {
  constexpr auto DICT_WINDOW = ((1 << LZSS_DICT_BITS) - LZSS_SEQ_MAX);
  BitWriter device;
  uint32_t in_i = 0;

  while (in_i < buffer.size()) {
    unsigned int seq_offset = 0;
    unsigned int seq_length = 0;
    unsigned int dict_i = ((in_i > DICT_WINDOW) ? (in_i - DICT_WINDOW) : 1);
    while ((dict_i < in_i) && (seq_length < LZSS_SEQ_MAX)) {
      unsigned int length_new = 0;
      while (length_new < LZSS_SEQ_MAX) {
        if ((in_i + length_new) >= buffer.size()) {
          break;
        }
        if (buffer[dict_i + length_new] != buffer[in_i + length_new]) {
          break;
        }
        length_new++;
      }
      if (length_new > seq_length) {
        if ((dict_i & LZSS_DICT_MASK) != LZSS_DICT_MASK) {
          seq_length = length_new;
          seq_offset = dict_i;
        }
      }
      dict_i++;
    }
    if (seq_length < LZSS_SEQ_MIN) {
      device.WriteBit(1U);
      device.WriteBits(buffer[in_i], 8);
      in_i++;
    } else {
      device.WriteBit(0U);
      device.WriteBits((seq_offset + 1), LZSS_DICT_BITS);
      device.WriteBits((seq_length - LZSS_SEQ_MIN), LZSS_SEQ_BITS);
      in_i += seq_length;
    }
  }
  device.WriteBit(0U);
  device.WriteBits(0, LZSS_DICT_BITS);
  return device.Buffer();
}

// ---- PbgArchive -------------------------------------------------------

uint32_t PbgArchive::EntryCount() const {
  return static_cast<uint32_t>(entries_.size());
}

PbgArchive PbgArchive::Open(const std::filesystem::path &path) {
  return FromBuffer(File_Load(path));
}

PbgArchive PbgArchive::Open(std::span<const uint8_t> bytes) {
  return FromBuffer(std::vector<uint8_t>(bytes.begin(), bytes.end()));
}

PbgArchive PbgArchive::FromBuffer(std::vector<uint8_t> packfile) {
  PbgArchive result;
  util::ByteReader reader{packfile};

  const auto head = reader.ReadObject<PbgHeader>();
  if (!head || head->name != kPbgHeadName) {
    return result;
  }

  std::vector<PbgEntryHeader> entries;
  if (head->n > reader.Remaining() / sizeof(PbgEntryHeader)) {
    return result;
  }
  entries.reserve(head->n);
  for (uint32_t i = 0; i < head->n; ++i) {
    const auto entry = reader.ReadObject<PbgEntryHeader>();
    if (!entry) {
      return result;
    }
    entries.push_back(*entry);
  }

  uint32_t total_checksum = 0;
  for (uint32_t i = 0; i < entries.size(); i++) {
    const auto maybe_compressed = CompressedEntry(packfile, entries, i);
    if (!maybe_compressed) {
      return result;
    }
    const auto checksum =
        std::accumulate(maybe_compressed.value().begin(),
                        maybe_compressed.value().end(), uint32_t{0});
    total_checksum += checksum;
    total_checksum += entries[i].size_uncompressed;
    total_checksum += entries[i].offset;
    if (checksum != entries[i].checksum_compressed) {
      return result;
    }
  }
  if (total_checksum != head->sum) {
    return result;
  }

  result.data_ = std::move(packfile);
  result.entries_ = std::move(entries);
  return result;
}

std::vector<uint8_t> PbgArchive::Extract(uint32_t index) const {
  const auto maybe_compressed = CompressedEntry(data_, entries_, index);
  if (!maybe_compressed) {
    return {};
  }
  return Decompress(maybe_compressed.value(),
                    entries_[index].size_uncompressed);
}

// ---- PbgArchiveWriter -----------------------------------------------------

void PbgArchiveWriter::Add(std::span<const uint8_t> data) {
  files_.emplace_back(data.begin(), data.end());
}

bool PbgArchiveWriter::Write(const std::filesystem::path &path) const {
  if (files_.size() > std::numeric_limits<uint32_t>::max() ||
      std::ranges::any_of(files_, [](const auto &file) {
        return file.size() > std::numeric_limits<uint32_t>::max();
      })) {
    return false;
  }
  PbgHeader head = {.n = static_cast<uint32_t>(files_.size())};
  std::vector<PbgEntryHeader> info(files_.size());
  uint32_t sum = 0;

  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream) {
    return false;
  }

  const auto write = [&stream](const void *data, size_t size) {
    if (static_cast<uintmax_t>(size) >
        static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
      return false;
    }
    stream.write(static_cast<const char *>(data),
                 static_cast<std::streamsize>(size));
    return static_cast<bool>(stream);
  };
  const auto write_header = [&] {
    return write(&head, sizeof(head)) &&
           write(info.data(), info.size() * sizeof(PbgEntryHeader));
  };

  const bool written = [&] {
    if (!write_header()) {
      return false;
    }

    for (size_t i = 0; i < files_.size(); i++) {
      auto compressed = Compress(files_[i]);
      const auto offset = stream.tellp();
      if (offset < 0 || static_cast<uintmax_t>(offset) >
                            std::numeric_limits<uint32_t>::max()) {
        return false;
      }
      info[i].offset = static_cast<uint32_t>(offset);
      info[i].size_uncompressed = static_cast<uint32_t>(files_[i].size());
      info[i].checksum_compressed = AccumulateEntryChecksum(
          sum, info[i].offset, info[i].size_uncompressed, compressed);
      if (!write(compressed.data(), compressed.size())) {
        return false;
      }
    }

    head.sum = sum;
    stream.seekp(0);
    if (!stream) {
      return false;
    }
    return write_header();
  }();

  stream.close();
  const bool closed = static_cast<bool>(stream);
  return written && closed;
}

} // namespace data
