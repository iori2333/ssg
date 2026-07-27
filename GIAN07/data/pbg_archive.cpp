///
/// PbgArchive - validated PBG packfile access
///
#include <array>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

#include <SDL3/SDL_iostream.h>

#include "pbg_archive.h"

#include "sys/bit_stream.h"
#include "sys/file.h"
#include "util/guard.h"

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

std::optional<BYTE_BUFFER_BORROWED>
CompressedEntry(const BYTE_BUFFER_OWNED &packfile,
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
  return BYTE_BUFFER_BORROWED{(packfile.get() + start), (end - start)};
}

static BYTE_BUFFER_OWNED Decompress(const BYTE_BUFFER_BORROWED &compressed,
                                    uint32_t size_uncompressed) {
  BYTE_BUFFER_OWNED uncompressed = {size_uncompressed};
  if (!uncompressed) {
    return nullptr;
  }

  std::array<uint8_t, (1 << LZSS_DICT_BITS)> dict{};
  uint32_t out_i = 0;

  auto output = [&](uint8_t literal) {
    uncompressed.get()[out_i] = literal;
    dict[out_i & LZSS_DICT_MASK] = literal;
    out_i++;
  };

  BitReader device{compressed};
  while (out_i < size_uncompressed) {
    if (!device.CanRead(1)) {
      return nullptr;
    }
    const bool is_literal = device.ReadBit() != 0U;
    if (is_literal) {
      if (!device.CanRead(8)) {
        return nullptr;
      }
      output(device.ReadBits(8));
    } else {
      if (!device.CanRead(LZSS_DICT_BITS)) {
        return nullptr;
      }
      auto seq_offset = device.ReadBits(LZSS_DICT_BITS);
      if (seq_offset == 0) {
        return nullptr;
      }
      seq_offset--;
      if (!device.CanRead(LZSS_SEQ_BITS)) {
        return nullptr;
      }
      const auto seq_length = (device.ReadBits(LZSS_SEQ_BITS) + LZSS_SEQ_MIN);
      if (seq_length > size_uncompressed - out_i) {
        return nullptr;
      }
      for (auto i = decltype(seq_length){0}; i < seq_length; i++) {
        output(dict[seq_offset++ & LZSS_DICT_MASK]);
      }
    }
  }
  return uncompressed;
}

BYTE_BUFFER_GROWABLE Compress(BYTE_BUFFER_BORROWED buffer) {
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

PbgArchive PbgArchive::Open(const char *path) {
  return FromBuffer(SDL_LoadFile(path));
}

PbgArchive PbgArchive::Open(SDL_IOStream *stream) {
  return FromBuffer(SDL_LoadFile_IO(stream, true));
}

PbgArchive PbgArchive::FromBuffer(BYTE_BUFFER_OWNED packfile) {
  PbgArchive result;
  auto packfile_cursor = packfile.cursor();

  const auto maybe_head = packfile_cursor.next<PbgHeader>();
  if (!maybe_head) {
    return result;
  }
  const auto &head = maybe_head.value()[0];
  if (head.name != kPbgHeadName) {
    return result;
  }

  const auto maybe_info = packfile_cursor.next<PbgEntryHeader>(head.n);
  if (!maybe_info) {
    return result;
  }
  const auto info_span = maybe_info.value();

  uint32_t total_checksum = 0;
  for (uint32_t i = 0; i < info_span.size(); i++) {
    const auto maybe_compressed = CompressedEntry(packfile, info_span, i);
    if (!maybe_compressed) {
      return result;
    }
    const auto checksum =
        std::accumulate(maybe_compressed.value().begin(),
                        maybe_compressed.value().end(), uint32_t{0});
    total_checksum += checksum;
    total_checksum += info_span[i].size_uncompressed;
    total_checksum += info_span[i].offset;
    if (checksum != info_span[i].checksum_compressed) {
      return result;
    }
  }
  if (total_checksum != head.sum) {
    return result;
  }

  result.data_ = std::move(packfile);
  result.entries_ = info_span;
  return result;
}

BYTE_BUFFER_OWNED PbgArchive::Extract(uint32_t index) const {
  const auto maybe_compressed = CompressedEntry(data_, entries_, index);
  if (!maybe_compressed) {
    return nullptr;
  }
  return Decompress(maybe_compressed.value(),
                    entries_[index].size_uncompressed);
}

// ---- PbgArchiveWriter -----------------------------------------------------

void PbgArchiveWriter::Add(std::span<const uint8_t> data) {
  files_.emplace_back(data.begin(), data.end());
}

bool PbgArchiveWriter::Write(
    const char *path, std::optional<FILE_TIMESTAMPS> maybe_timestamps) const {
  PbgHeader head = {.n = static_cast<uint32_t>(files_.size())};
  std::vector<PbgEntryHeader> info(files_.size());
  uint32_t sum = 0;

  auto write_header = [&head, &info](SDL_IOStream *stream) {
    return (SDL_MustWriteIO(stream, &head, sizeof(head)) &&
            SDL_MustWriteIO(stream, info.data(),
                            (info.size() * sizeof(PbgEntryHeader))));
  };

  auto *stream = SDL_IOFromFile(path, "wb");
  if (stream == nullptr) {
    return false;
  }
  auto close_guard = make_guard([&] {
    File_CloseWithTimestamps(std::move(stream), path,
                             std::move(maybe_timestamps));
  });

  if (!write_header(stream)) {
    return false;
  }

  for (uint32_t i = 0; i < files_.size(); i++) {
    auto compressed = Compress(BYTE_BUFFER_BORROWED{files_[i]});

    const auto offset = SDL_TellIO(stream);
    if (offset == -1) {
      return false;
    }
    info[i].offset = offset;
    info[i].size_uncompressed = static_cast<uint32_t>(files_[i].size());
    info[i].checksum_compressed = AccumulateEntryChecksum(
        sum, info[i].offset, info[i].size_uncompressed, compressed);
    if (!SDL_MustWriteIO(stream, compressed.data(), compressed.size())) {
      return false;
    }
  }

  head.sum = sum;
  return ((SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET) != -1) &&
          write_header(stream));
}

} // namespace data
