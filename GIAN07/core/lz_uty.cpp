///
/// LzUty - Packfiles and compression
///
#include "lz_uty.h"

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <vector>

#include <SDL3/SDL_iostream.h>

#include "lz_uty.h"

#include "sys/file.h"
#include "util/guard.h"

namespace {

constexpr auto LZSS_DICT_BITS = 13;
constexpr auto LZSS_SEQ_BITS = 4;
constexpr auto LZSS_SEQ_MIN = 3;
constexpr auto LZSS_DICT_MASK = ((1 << LZSS_DICT_BITS) - 1);
constexpr auto LZSS_SEQ_MAX = (LZSS_SEQ_MIN + ((1 << LZSS_SEQ_BITS) - 1));

constexpr const std::array<char, 4> kPbgHeadName = {'P', 'B', 'G', 0x1A};

struct PbgFileHead {
  std::array<char, kPbgHeadName.size()> name = kPbgHeadName;
  ENDIAN_LITTLE<uint32_t> sum = 0;
  ENDIAN_LITTLE<uint32_t> n = 0;
};

} // namespace

// ---- Bit-level I/O --------------------------------------------------

uint8_t BIT_DEVICE_READ::GetBit() {
  if (cursor.byte >= buffer.size()) {
    return 0xFF;
  }
  const bool ret = ((buffer[cursor.byte] >> (7 - cursor.bit)) & 1) != 0;
  cursor += 1;
  return static_cast<uint8_t>(ret);
}

uint32_t BIT_DEVICE_READ::GetBits(size_t bitcount) {
  const auto bytes_remaining = (buffer.size() - cursor.byte);
  if ((bitcount > 24) || (bytes_remaining == 0)) {
    return 0xFFFFFFFF;
  }
  if (((bitcount + 7) / 8) >= bytes_remaining) {
    bitcount = std::min(((bytes_remaining * 8) - cursor.bit), bitcount);
  }
  const auto window_size = (cursor.bit + bitcount);

  uint32_t window = (buffer[cursor.byte + 0] << 24);
  if ((bitcount > 1) && (window_size > 8)) {
    window |= (buffer[cursor.byte + 1] << 16);
  }
  if ((bitcount > 9) && (window_size > 16)) {
    window |= (buffer[cursor.byte + 2] << 8);
  }
  if ((bitcount > 17) && (window_size > 24)) {
    window |= (buffer[cursor.byte + 3] << 0);
  }
  window <<= cursor.bit;
  cursor += bitcount;
  return (window >> (32 - bitcount));
}

void BIT_DEVICE_WRITE::PutBit(uint8_t bit) {
  if (bit_cursor == 0) {
    buffer.push_back(0x00);
  }
  buffer.back() |= ((bit & 1) << (7 - bit_cursor));
  if (++bit_cursor == 0) {
    buffer.push_back(0x00);
  }
}

void BIT_DEVICE_WRITE::PutBits(uint32_t bits, unsigned int bitcount) {
  uint32_t mask = (1 << (bitcount - 1));
  for (decltype(bitcount) i = 0; i < bitcount; i++) {
    PutBit(static_cast<uint8_t>((bits & mask) != 0));
    mask >>= 1;
  }
}

bool BIT_DEVICE_WRITE::Write(const char *s) const {
  return SDL_SaveFile(s, buffer.data(), buffer.size());
}

BIT_FILE_READ BitFilCreateR(const char *s) { return {SDL_LoadFile(s)}; }

// ---- LZSS -----------------------------------------------------------

template <typename Container>
uint32_t FilChecksumAddFile(uint32_t &current_total, uint32_t offset,
                            uint32_t size_uncompressed,
                            const Container &compressed) {
  auto ret = std::accumulate(compressed.begin(), compressed.end(), uint32_t{0});
  current_total += ret;
  current_total += size_uncompressed;
  current_total += offset;
  return ret;
}

std::optional<BYTE_BUFFER_BORROWED>
FilFileGetCompressed(const BYTE_BUFFER_OWNED &packfile,
                     std::span<const PbgFileInfo> info, uint32_t filno) {
  if (filno >= info.size()) {
    return std::nullopt;
  }
  const size_t start = info[filno].offset;
  const auto end = ((filno == (info.size() - 1))
                        ? packfile.size()
                        : size_t{info[filno + 1].offset});
  if ((start >= packfile.size()) || (end > packfile.size())) {
    return std::nullopt;
  }
  return BYTE_BUFFER_BORROWED{(packfile.get() + start), (end - start)};
}

static BYTE_BUFFER_OWNED
Decompress(const BYTE_BUFFER_BORROWED &compressed, uint32_t size_uncompressed) {
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

  BIT_DEVICE_READ device = {compressed};
  while (out_i < size_uncompressed) {
    const bool is_literal = device.GetBit() != 0U;
    if (is_literal) {
      output(device.GetBits(8));
    } else {
      auto seq_offset = device.GetBits(LZSS_DICT_BITS);
      if (seq_offset == 0) {
        break;
      }
      seq_offset--;
      const auto seq_length = (device.GetBits(LZSS_SEQ_BITS) + LZSS_SEQ_MIN);
      for (auto i = decltype(seq_length){0}; i < seq_length; i++) {
        output(dict[seq_offset++ & LZSS_DICT_MASK]);
      }
    }
  }
  return uncompressed;
}

BYTE_BUFFER_GROWABLE Compress(BYTE_BUFFER_BORROWED buffer) {
  constexpr auto DICT_WINDOW = ((1 << LZSS_DICT_BITS) - LZSS_SEQ_MAX);
  BIT_DEVICE_WRITE device;
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
      device.PutBit(1U);
      device.PutBits(buffer[in_i], 8);
      in_i++;
    } else {
      device.PutBit(0U);
      device.PutBits((seq_offset + 1), LZSS_DICT_BITS);
      device.PutBits((seq_length - LZSS_SEQ_MIN), LZSS_SEQ_BITS);
      in_i += seq_length;
    }
  }
  device.PutBit(0U);
  device.PutBits(0, LZSS_DICT_BITS);
  return device.buffer;
}

// ---- PackFile -------------------------------------------------------

PackFile OpenFromBuffer(BYTE_BUFFER_OWNED);

uint32_t PackFile::Count() const {
  return static_cast<uint32_t>(entries_.size());
}

PackFile PackFile::Open(const char *path) {
  return OpenFromBuffer(SDL_LoadFile(path));
}

PackFile PackFile::Open(SDL_IOStream &stream) {
  return OpenFromBuffer(SDL_LoadFile_IO(&stream, true));
}

PackFile OpenFromBuffer(BYTE_BUFFER_OWNED packfile) {
  PackFile result;
  auto packfile_cursor = packfile.cursor();

  const auto maybe_head = packfile_cursor.next<PbgFileHead>();
  if (!maybe_head) {
    return result;
  }
  const auto &head = maybe_head.value()[0];
  if (head.name != kPbgHeadName) {
    return result;
  }

  const auto maybe_info = packfile_cursor.next<PbgFileInfo>(head.n);
  if (!maybe_info) {
    return result;
  }
  const auto info_span = maybe_info.value();

  uint32_t total_checksum = 0;
  for (uint32_t i = 0; i < info_span.size(); i++) {
    const auto maybe_compressed =
        FilFileGetCompressed(packfile, info_span, i);
    if (!maybe_compressed) {
      return result;
    }
    const auto checksum = std::accumulate(
        maybe_compressed.value().begin(), maybe_compressed.value().end(),
        uint32_t{0});
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

BYTE_BUFFER_OWNED PackFile::Extract(uint32_t index) const {
  const auto maybe_compressed = FilFileGetCompressed(data_, entries_, index);
  if (!maybe_compressed) {
    return nullptr;
  }
  return Decompress(maybe_compressed.value(),
                    entries_[index].size_uncompressed);
}

// ---- PackWriter -----------------------------------------------------

void PackWriter::Add(std::span<const uint8_t> data) {
  files_.emplace_back(data.data(), data.size());
}

bool PackWriter::Write(const char *path,
                       std::optional<FILE_TIMESTAMPS> maybe_timestamps) const {
  PbgFileHead head = {.n = static_cast<uint32_t>(files_.size())};
  std::vector<PbgFileInfo> info(files_.size());
  uint32_t sum = 0;

  auto write_header = [&head, &info](SDL_IOStream *stream) {
    return (SDL_MustWriteIO(stream, &head, sizeof(head)) &&
            SDL_MustWriteIO(stream, info.data(),
                            (info.size() * sizeof(PbgFileInfo))));
  };

  auto *stream = SDL_IOFromFile(path, "wb");
  if (stream == nullptr) {
    return false;
  }
  auto close_guard = make_guard([&] {
    File_CloseWithTimestamps(std::move(stream), path, std::move(maybe_timestamps));
  });

  if (!write_header(stream)) {
    return false;
  }

  for (uint32_t i = 0; i < files_.size(); i++) {
    auto compressed = Compress(files_[i]);

    const auto offset = SDL_TellIO(stream);
    if (offset == -1) {
      return false;
    }
    info[i].offset = offset;
    info[i].size_uncompressed =
        static_cast<uint32_t>(files_[i].size());
    info[i].checksum_compressed = FilChecksumAddFile(
        sum, info[i].offset, info[i].size_uncompressed, compressed);
    if (!SDL_MustWriteIO(stream, compressed.data(), compressed.size())) {
      return false;
    }
  }

  head.sum = sum;
  return ((SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET) != -1) &&
          write_header(stream));
}