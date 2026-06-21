///
/// pack_tool - GIAN07 pack file modification tool
///
/// Usage:
///   pack_tool extract <packfile> <out_dir>
///   pack_tool pack <in_dir> <packfile>
///   pack_tool strip <in_packfile> <out_packfile>
///   pack_tool extract-music <data_dir> <out_dir>
///   pack_tool pack-music <in_dir> <out_packfile>
///   pack_tool scl <in_file> <multiplier> <out_file>
///   pack_tool ecl <in_file> <multiplier> <out_file>
///   pack_tool ecl-time <in_file> <script_id> <multiplier> <out_file>
///   pack_tool ecl-boss <in_file> <script_id> <hp_mult> <timer_mult> <out_file>
///   pack_tool dump-scl <in_file>
///   pack_tool dump-ecl <in_file> [script_id]
///   pack_tool patch4 <in_file> <offset_hex> <value_hex> <out_file>
///

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <iconv.h>
#endif

#include "core/lz_uty.h"
#include "util/endian.h"

namespace fs = std::filesystem;

namespace fs = std::filesystem;

// ============================================================================
// Command name tables
// ============================================================================

static const char *scl_name(uint8_t op) {
  switch (op) {
  case 0x00: return "TIME";
  case 0x01: return "ENEMY";
  case 0x02: return "SSP";
  case 0x03: return "EFC";
  case 0x04: return "END";
  case 0x05: return "BOSS";
  case 0x06: return "MWOPEN";
  case 0x07: return "MWCLOSE";
  case 0x08: return "MSG";
  case 0x09: return "KEY";
  case 0x0A: return "NPG";
  case 0x0B: return "FACE";
  case 0x0C: return "MUSIC";
  case 0x0D: return "BOSSDEAD";
  case 0x0E: return "LOADFACE";
  case 0x0F: return "WAITEX";
  case 0x10: return "STAGECLEAR";
  case 0x11: return "MAPPALETTE";
  case 0x12: return "GAMECLEAR";
  case 0x13: return "DELENEMY";
  case 0x14: return "ENEMYPALETTE";
  case 0x15: return "STAFF";
  case 0x16: return "EXTRACLEAR";
  default:  return nullptr;
  }
}

static const char *ecl_name(uint8_t op) {
  switch (op) {
  case 0x00: return "SETUP";
  case 0x01: return "END";
  case 0x02: return "JMP";
  case 0x03: return "LOOP";
  case 0x04: return "CALL";
  case 0x05: return "RET";
  case 0x06: return "JHPL";
  case 0x07: return "JHPS";
  case 0x08: return "JDIF";
  case 0x09: return "JDSB";
  case 0x0A: return "JFCL";
  case 0x0B: return "JFCS";
  case 0x0C: return "STI";
  case 0x0D: return "CLI";
  case 0x10: return "NOP";
  case 0x11: return "NOPSC";
  case 0x12: return "MOV";
  case 0x13: return "ROL";
  case 0x14: return "LROL";
  case 0x15: return "WAVX";
  case 0x16: return "WAVY";
  case 0x17: return "MXA";
  case 0x18: return "MYA";
  case 0x19: return "MXYA";
  case 0x1A: return "MXS";
  case 0x1B: return "MYS";
  case 0x1C: return "MXYS";
  case 0x1D: return "ACC";
  case 0x1E: return "ACCXYA";
  case 0x1F: return "GRAX";
  case 0x20: return "DEGA";
  case 0x21: return "DEGR";
  case 0x22: return "DEGX";
  case 0x23: return "DEGS";
  case 0x24: return "SPDA";
  case 0x25: return "SPDR";
  case 0x26: return "XYA";
  case 0x27: return "XYR";
  case 0x28: return "DEGXU";
  case 0x29: return "DEGXD";
  case 0x2A: return "DEGEX";
  case 0x2B: return "XYS";
  case 0x2C: return "DEGX2";
  case 0x2D: return "XYRND";
  case 0x2E: return "XYL";
  case 0x40: return "TAMA";
  case 0x41: return "TAUTO";
  case 0x42: return "TXYR";
  case 0x43: return "TCMD";
  case 0x44: return "TDEGA";
  case 0x45: return "TDEGR";
  case 0x46: return "TNUMA";
  case 0x47: return "TNUMR";
  case 0x48: return "TSPDA";
  case 0x49: return "TSPDR";
  case 0x4A: return "TOPT";
  case 0x4B: return "TTYPE";
  case 0x4C: return "TCOL";
  case 0x4D: return "TVDEG";
  case 0x4E: return "TREP";
  case 0x4F: return "TDEGS";
  case 0x50: return "TDEGE";
  case 0x51: return "TAMA2";
  case 0x52: return "TCLR";
  case 0x53: return "TAMAL";
  case 0x54: return "T2ITEM";
  case 0x55: return "TAMAEX";
  case 0x60: return "LASER";
  case 0x61: return "LCMD";
  case 0x62: return "LLA";
  case 0x63: return "LLR";
  case 0x64: return "LL2";
  case 0x65: return "LDEGA";
  case 0x66: return "LDEGR";
  case 0x67: return "LNUMA";
  case 0x68: return "LNUMR";
  case 0x69: return "LSPDA";
  case 0x6A: return "LSPDR";
  case 0x6B: return "LCOL";
  case 0x6C: return "LTYPE";
  case 0x6D: return "LWA";
  case 0x6E: return "LDEGS";
  case 0x6F: return "LDEGE";
  case 0x70: return "LXY";
  case 0x71: return "LASER2";
  case 0x80: return "LLSET";
  case 0x81: return "LLOPEN";
  case 0x82: return "LLCLOSE";
  case 0x83: return "LLCLOSEL";
  case 0x84: return "LLDEGR";
  case 0x85: return "HLASER";
  case 0x90: return "DRAW_ON";
  case 0x91: return "DRAW_OFF";
  case 0x92: return "CLIP_ON";
  case 0x93: return "CLIP_OFF";
  case 0x94: return "DAMAGE_ON";
  case 0x95: return "DAMAGE_OFF";
  case 0x96: return "HITSB_ON";
  case 0x97: return "HITSB_OFF";
  case 0x98: return "RLCHG_ON";
  case 0x99: return "RLCHG_OFF";
  case 0xA0: return "ANM";
  case 0xA1: return "PSE";
  case 0xA2: return "INT";
  case 0xA3: return "EXDEGD";
  case 0xA4: return "ENEMYSET";
  case 0xA5: return "ENEMYSETD";
  case 0xA6: return "HITXY";
  case 0xA7: return "ITEM";
  case 0xA8: return "STG4EFC";
  case 0xA9: return "ANMEX";
  case 0xAA: return "BITLASER";
  case 0xAB: return "BITATTACK";
  case 0xAC: return "BITCMD";
  case 0xAD: return "BOSSSET";
  case 0xAE: return "CEFC";
  case 0xAF: return "STG3EFC";
  case 0xB0: return "MOVR";
  case 0xB1: return "MOVC";
  case 0xB2: return "ADD";
  case 0xB3: return "SUB";
  case 0xB4: return "SINL";
  case 0xB5: return "COSL";
  case 0xB6: return "MOD";
  case 0xB7: return "RND";
  case 0xB8: return "CMPR";
  case 0xB9: return "CMPC";
  case 0xBA: return "JL";
  case 0xBB: return "JS";
  case 0xBC: return "INC";
  case 0xBD: return "DEC";
  case 0xBE: return "JEQ";
  default:  return nullptr;
  }
}

// ECL command lengths (from ecl_len.h)
static const uint8_t ecl_cmd_len[256] = {
    9,  1,  5,  7,  5,  1,  9,  9,  17, 5,  9,  9,  10, 2,  0,  0,
    3,  3,  3,  4,  12, 9,  9,  5,  5,  7,  3,  3,  3,  4,  7,  2,
    2,  2,  1,  1,  5,  5,  5,  5,  1,  1,  1,  1,  1,  1,  3,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  5,  2,  3,  3,  3,  3,  3,  3,  2,  2,  2,  2,  2,  1,
    1,  1,  1,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  5,  5,  5,  3,  3,  2,  2,  5,  5,  2,  2,  5,  1,  1,
    5,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  2,  2,  2,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,
    3,  2,  2,  2,  6,  7,  5,  2,  2,  2,  2,  5,  6,  2,  6,  1,
    3,  6,  3,  3,  3,  3,  6,  2,  3,  6,  5,  5,  2,  2,  5,  0,
};

// ============================================================================
// Helpers
// ============================================================================

static void print_usage() {
  std::println(stderr, R"(pack_tool - GIAN07 pack file tool

Usage:
  pack_tool extract <packfile> <out_dir>
      Extract all entries from a PBG pack file to out_dir/NNN.bin

  pack_tool pack <in_dir> <packfile>
      Repack all NNN.bin files from in_dir into a PBG pack file

  pack_tool strip <in_packfile> <out_packfile>
      Strip ECL/SCL script and music room comment entries from an old
      48-entry ENEMY.DAT, keeping only map + demo data (13 entries)

  pack_tool extract-music <data_dir> <out_dir>
      Extract MIDI tracks, titles (GBK→UTF-8), and comments (GBK→UTF-8)
      from MUSIC.DAT + legacy ENEMY.DAT into out_dir/track_NN/

  pack_tool pack-music <in_dir> <out_packfile>
      Pack unified MUSIC.PAK from track_NN/ directories

  pack_tool scl <in_file> <multiplier> <out_file>
      Multiply the end-boss SCL_TIME frame value by multiplier

  pack_tool ecl <in_file> <multiplier> <out_file>
      Multiply all ECL SETUP HP values by multiplier

  pack_tool ecl-time <in_file> <script_id> <multiplier> <out_file>
      Multiply STI TIMER values for a specific ECL script

  pack_tool dump-scl <in_file>
      Print SCL commands with names and values

  pack_tool dump-ecl <in_file>
      Print ECL header and SETUP HP/score + STI commands for each script
)");
}

static std::vector<uint8_t> read_file(const char *path) {
  auto *fp = std::fopen(path, "rb");
  if (!fp) {
    std::println(stderr, "Error: Cannot open '{}'", path);
    return {};
  }
  std::fseek(fp, 0, SEEK_END);
  const auto size = std::ftell(fp);
  std::fseek(fp, 0, SEEK_SET);
  std::vector<uint8_t> buf(size);
  if (std::fread(buf.data(), 1, size, fp) != static_cast<size_t>(size)) {
    std::println(stderr, "Error: Failed to read '{}'", path);
    std::fclose(fp);
    return {};
  }
  std::fclose(fp);
  return buf;
}

static bool write_file(const char *path,
                       const std::vector<uint8_t> &data) {
  auto *fp = std::fopen(path, "wb");
  if (!fp) {
    std::println(stderr, "Error: Cannot write '{}'", path);
    return false;
  }
  if (std::fwrite(data.data(), 1, data.size(), fp) != data.size()) {
    std::println(stderr, "Error: Failed to write '{}'", path);
    std::fclose(fp);
    return false;
  }
  std::fclose(fp);
  return true;
}

static bool write_file(const char *path, const uint8_t *data, size_t size) {
  auto *fp = std::fopen(path, "wb");
  if (!fp) {
    std::println(stderr, "Error: Cannot write '{}'", path);
    return false;
  }
  if (std::fwrite(data, 1, size, fp) != size) {
    std::println(stderr, "Error: Failed to write '{}'", path);
    std::fclose(fp);
    return false;
  }
  std::fclose(fp);
  return true;
}

// ============================================================================
// GBK → UTF-8 conversion
// ============================================================================

#ifdef WIN32
static std::string gbk_to_utf8(std::string_view gbk) {
  if (gbk.empty()) {
    return {};
  }
  const int wide_len = MultiByteToWideChar(936, MB_ERR_INVALID_CHARS,
                                           gbk.data(), (int)gbk.size(),
                                           nullptr, 0);
  if (wide_len <= 0) {
    std::println(stderr, "Warning: GBK → UTF-8 conversion failed for input");
    return std::string(gbk);
  }
  std::vector<wchar_t> wide(wide_len);
  MultiByteToWideChar(936, MB_ERR_INVALID_CHARS,
                      gbk.data(), (int)gbk.size(), wide.data(), wide_len);
  const int utf8_len = WideCharToMultiByte(CP_UTF8, 0,
                                           wide.data(), wide_len,
                                           nullptr, 0, nullptr, nullptr);
  if (utf8_len <= 0) {
    return std::string(gbk);
  }
  std::string utf8(utf8_len, '\0');
  WideCharToMultiByte(CP_UTF8, 0,
                      wide.data(), wide_len,
                      utf8.data(), utf8_len, nullptr, nullptr);
  return utf8;
}
#else
static std::string gbk_to_utf8(std::string_view gbk) {
  if (gbk.empty()) {
    return {};
  }
  iconv_t cd = iconv_open("UTF-8", "GBK");
  if (cd == (iconv_t)-1) {
    std::println(stderr, "Warning: iconv_open(UTF-8, GBK) failed");
    return std::string(gbk);
  }

  auto cleanup = [&] { iconv_close(cd); };
  std::string out;
  out.resize(gbk.size() * 3 / 2 + 4);

  char *inbuf = const_cast<char *>(gbk.data());
  size_t inbytes = gbk.size();
  char *outbuf = out.data();
  size_t outbytes = out.size();

  while (inbytes > 0) {
    size_t ret = iconv(cd, &inbuf, &inbytes, &outbuf, &outbytes);
    if (ret == (size_t)-1) {
      if (errno == E2BIG) {
        const auto written = outbuf - out.data();
        out.resize(out.size() * 2);
        outbuf = out.data() + written;
        outbytes = out.size() - written;
        continue;
      }
      break;
    }
  }

  out.resize(outbuf - out.data());
  cleanup();
  return out;
}
#endif

// ============================================================================
// SMF (Standard MIDI File) title extractor
// ============================================================================

static uint32_t read_vlq(const uint8_t *&pos, const uint8_t *end) {
  uint32_t val = 0;
  int bytes = 0;
  while ((pos < end) && (bytes < 4)) {
    const uint8_t b = *pos++;
    bytes++;
    val = (val << 7) | (b & 0x7F);
    if (!(b & 0x80)) {
      return val;
    }
  }
  return val;
}

static int midi_status_data_len(uint8_t status) {
  switch (status & 0xF0) {
  case 0x80:
  case 0x90:
  case 0xA0:
  case 0xB0:
  case 0xE0:
    return 2;
  case 0xC0:
  case 0xD0:
    return 1;
  default:
    return 0;
  }
}

static std::string extract_smf_title(const std::vector<uint8_t> &data) {
  auto end = data.data() + data.size();
  auto pos = data.data();

  if ((end - pos) < 14)
    return {};
  if (std::memcmp(pos, "MThd", 4) != 0)
    return {};
  pos += 4;
  const uint32_t hdr_len =
      ((uint32_t)((uint8_t)pos[0]) << 24) |
      ((uint32_t)((uint8_t)pos[1]) << 16) |
      ((uint32_t)((uint8_t)pos[2]) << 8) |
      ((uint32_t)((uint8_t)pos[3]));
  pos += 4;
  pos += hdr_len;

  std::string fallback_title;

  while (end - pos >= 8) {
    if (std::memcmp(pos, "MTrk", 4) != 0) {
      pos++;
      continue;
    }
    pos += 4;
    const uint32_t trk_len =
        ((uint32_t)((uint8_t)pos[0]) << 24) |
        ((uint32_t)((uint8_t)pos[1]) << 16) |
        ((uint32_t)((uint8_t)pos[2]) << 8) |
        ((uint32_t)((uint8_t)pos[3]));
    pos += 4;

    const auto *trk_end = pos + trk_len;
    if (trk_end > end)
      trk_end = end;

    uint8_t running_status = 0;

    while (pos < trk_end) {
      read_vlq(pos, trk_end);
      if (pos >= trk_end)
        break;

      uint8_t byte = *pos++;

      if (byte == 0xFF) {
        if (pos >= trk_end)
          break;
        const uint8_t meta = *pos++;
        const uint32_t len = read_vlq(pos, trk_end);

        if (meta == 0x03) {
          if (pos + len <= trk_end) {
            return std::string(reinterpret_cast<const char *>(pos), len);
          }
          return {};
        }
        if (meta == 0x01 && fallback_title.empty()) {
          if (pos + len <= trk_end) {
            fallback_title =
                std::string(reinterpret_cast<const char *>(pos), len);
          }
        }
        pos += std::min(len, (uint32_t)(trk_end - pos));
      } else if ((byte & 0xF0) == 0xF0) {
        if (byte == 0xFF) {
          if (pos >= trk_end)
            break;
          pos++;
        }
        const uint32_t len = read_vlq(pos, trk_end);
        pos += std::min(len, (uint32_t)(trk_end - pos));
      } else {
        if (!(byte & 0x80)) {
          pos--;
          byte = running_status;
        } else {
          running_status = byte;
        }
        const int data_len = midi_status_data_len(byte);
        if (pos + data_len <= trk_end) {
          pos += data_len;
        } else {
          pos = trk_end;
        }
      }
    }
  }

  return fallback_title;
}

// ============================================================================
// Music comment parser (fixed 38-byte GBK lines → UTF-8, \n-separated)
// ============================================================================

static std::string parse_music_comment(const std::vector<uint8_t> &data) {
  static constexpr size_t LINE_BYTES = 38;
  std::string result;
  bool first = true;

  for (size_t offset = 0; offset + LINE_BYTES <= data.size();
       offset += LINE_BYTES) {
    const auto *line = data.data() + offset;
    const size_t len = strnlen(reinterpret_cast<const char *>(line),
                                LINE_BYTES);
    if (len == 0)
      break;

    if (first) {
      first = false;
      continue;
    }

    auto utf8 = gbk_to_utf8(
        std::string_view(reinterpret_cast<const char *>(line), len));
    if (utf8.empty()) {
      utf8 =
          std::string(reinterpret_cast<const char *>(line), std::min(len, 1UZ));
    }
    result += utf8;
    result += '\n';
  }

  while (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }
  return result;
}

// ============================================================================
// String file I/O
// ============================================================================

static std::string read_text_file(const fs::path &path) {
  auto bytes = read_file(path.string().c_str());
  return {reinterpret_cast<const char *>(bytes.data()), bytes.size()};
}

static bool write_text_file(const fs::path &path, std::string_view text) {
  return write_file(path.string().c_str(),
                    reinterpret_cast<const uint8_t *>(text.data()), text.size());
}

// ============================================================================
// Extract mode
// ============================================================================

static bool cmd_extract(const char *packfile, const char *out_dir) {
  auto reader = FilStartR(packfile);
  if (!reader) {
    std::println(stderr, "Error: Cannot open pack file '{}'", packfile);
    return false;
  }

  std::error_code ec;
  fs::create_directories(out_dir, ec);

  const auto n = static_cast<int>(reader.info.size());
  std::println("Extracting {} entries from '{}' to '{}'...", n, packfile,
               out_dir);

  for (int i = 0; i < n; i++) {
    auto data = reader.MemExpand(i);
    if (!data) {
      std::println(stderr, "Error: Failed to decompress entry {}", i);
      return false;
    }
    const auto out_path = std::format("{}\\{:03}.bin", out_dir, i);
    if (!write_file(out_path.c_str(), data.get(), data.size())) {
      return false;
    }
    std::println("  {:03}.bin  {} bytes", i, data.size());
  }

  std::println("Done.");
  return true;
}

// ============================================================================
// Pack mode
// ============================================================================

static bool cmd_pack(const char *in_dir, const char *packfile) {
  std::error_code ec;
  if (!fs::is_directory(in_dir, ec)) {
    std::println(stderr, "Error: '{}' is not a directory", in_dir);
    return false;
  }

  // Collect .bin files sorted by numeric prefix
  std::vector<fs::path> files;
  for (const auto &entry : fs::directory_iterator(in_dir)) {
    if (entry.is_regular_file() && entry.path().extension() == ".bin") {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());

  if (files.empty()) {
    std::println(stderr, "Error: No .bin files found in '{}'", in_dir);
    return false;
  }

  std::println("Packing {} files to '{}'...", files.size(), packfile);

  PACKFILE_WRITE writer;
  std::vector<std::vector<uint8_t>> buffers;
  buffers.reserve(files.size());

  for (const auto &path : files) {
    auto buf = read_file(path.string().c_str());
    if (buf.empty()) {
      return false;
    }
    std::println("  {}  {} bytes", path.filename().string(), buf.size());
    buffers.emplace_back(std::move(buf));
  }

  for (auto &buf : buffers) {
    writer.files.emplace_back(buf.data(), buf.size());
  }

  if (!writer.Write(packfile)) {
    std::println(stderr, "Error: Failed to write pack file '{}'", packfile);
    return false;
  }

  std::println("Done.");
  return true;
}

// ============================================================================
// SCL modify mode
// ============================================================================

static bool cmd_scl(const char *in_file, float mult, const char *out_file) {
  if (mult <= 0.0f) {
    std::println(stderr, "Error: Multiplier must be positive");
    return false;
  }

  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }

  // Find the last SCL_STAGECLEAR (0x10) — this marks the end of the stage.
  int stageclear = -1;
  for (int i = static_cast<int>(data.size()) - 1; i >= 0; i--) {
    if (data[i] == 0x10) {
      stageclear = i;
      break;
    }
  }
  if (stageclear < 0) {
    std::println(stderr,
                 "Error: No SCL_STAGECLEAR (0x10) found "
                 "- this may not be an SCL file");
    return false;
  }

  // Walk backward from STAGECLEAR to find the stage-boss SCL_BOSSDEAD.
  // A 0x0D byte is considered a valid BOSSDEAD only if the byte before it
  // is not a command opcode that embeds 0x0D as a parameter.
  int bossdead = -1;
  for (int i = stageclear - 1; i >= 0; i--) {
    if (data[i] != 0x0D) {
      continue;
    }
    // Reject if preceded by a command that would embed 0x0D as data:
    //   0x0B = SCL_FACE (face_id), 0x0C = SCL_MUSIC (music_id),
    //   0x03 = SCL_EFC (efc_type)
    if (i >= 1 && (data[i - 1] == 0x0B || data[i - 1] == 0x0C ||
                   data[i - 1] == 0x03)) {
      continue;
    }
    bossdead = i;
    break;
  }
  if (bossdead < 0) {
    std::println(stderr,
                 "Error: No valid SCL_BOSSDEAD before SCL_STAGECLEAR");
    return false;
  }

  // Walk backward from BOSSDEAD to find the stage-boss SCL_TIME.
  // Frame value must be in a reasonable range for a boss timeout.
  constexpr uint32_t MIN_FRAME = 500;
  constexpr uint32_t MAX_FRAME = 20000;

  int time_pos = -1;
  uint32_t time_val = 0;
  for (int i = bossdead - 1; i >= 4; i--) {
    if (data[i] != 0x00) {
      continue;
    }
    // Reject if preceded by a command that embeds 0x00:
    //   0x0B = SCL_FACE, 0x0C = SCL_MUSIC
    if (i >= 1 && (data[i - 1] == 0x0B || data[i - 1] == 0x0C)) {
      continue;
    }
    const auto val = U32LEAt(&data[i + 1]);
    if (val >= MIN_FRAME && val <= MAX_FRAME) {
      time_pos = i;
      time_val = val;
      break;
    }
  }
  if (time_pos < 0) {
    std::println(stderr,
                 "Error: No reasonable SCL_TIME before stage BOSSDEAD");
    return false;
  }

  const auto new_val = static_cast<uint32_t>(
      std::llround(static_cast<double>(time_val) * mult));
  *reinterpret_cast<U32LE *>(&data[time_pos + 1]) = U32LE(new_val);

  std::println("SCL_STAGECLEAR at +0x{:04X}, BOSSDEAD at +0x{:04X}",
               stageclear, bossdead);
  std::println("Modified SCL_TIME at +0x{:04X}: {} -> {} (x{:.1f})",
               time_pos, time_val, new_val, mult);

  return write_file(out_file, data);
}

// ============================================================================
// ECL modify mode
// ============================================================================

static bool cmd_ecl(const char *in_file, float mult, const char *out_file) {
  if (mult <= 0.0f) {
    std::println(stderr, "Error: Multiplier must be positive");
    return false;
  }

  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }

  if (data.size() < 4) {
    std::println(stderr, "Error: ECL file too small (no header)");
    return false;
  }

  const auto script_count = U32LEAt(data.data());
  if (script_count == 0 || script_count > 256) {
    std::println(stderr, "Error: Invalid script count {} in ECL header",
                 static_cast<uint32_t>(script_count));
    return false;
  }

  const size_t header_size = 4 + static_cast<size_t>(script_count) * 4;
  if (data.size() < header_size) {
    std::println(stderr, "Error: ECL file too small (header truncated)");
    return false;
  }

  int modified = 0;
  std::unordered_set<uint32_t> seen_offsets;
  for (uint32_t si = 0; si < script_count; si++) {
    const auto offset = U32LEAt(data.data() + 4 + si * 4);
    if (offset >= data.size() || (offset + 9) > data.size()) {
      std::println(stderr, "  Script {}: offset {:#x} out of bounds, skipping",
                   si, static_cast<uint32_t>(offset));
      continue;
    }
    if (seen_offsets.contains(offset)) {
      continue; // already modified (shared script)
    }
    seen_offsets.insert(offset);

    const uint8_t opcode = data[offset];
    if (opcode != 0x00) {
      std::println(
          stderr,
          "  Script {}: first byte is {:#04x} (expected SETUP 0x00), skipping",
          si, opcode);
      continue;
    }

    const auto old_hp = U32LEAt(&data[offset + 1]);
    const auto new_hp =
        static_cast<uint32_t>(std::llround(static_cast<double>(old_hp) * mult));
    *reinterpret_cast<U32LE *>(&data[offset + 1]) = U32LE(new_hp);

    std::println("  Script {} at +0x{:04X}: HP {} -> {} (x{:.1f})", si,
                 static_cast<uint32_t>(offset), static_cast<uint32_t>(old_hp),
                 new_hp, mult);
    modified++;
  }

  std::println("Modified {} script(s).", modified);

  return write_file(out_file, data);
}

// ============================================================================
// dump-scl mode
// ============================================================================

static bool cmd_dump_scl(const char *in_file) {
  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }

  size_t pos = 0;
  int lineno = 0;
  while (pos < data.size()) {
    const uint8_t op = data[pos];
    const auto *name = scl_name(op);

    if (op == 0x0D) {
      std::println("  {:4d}  +0x{:04X}  BOSSDEAD", lineno, pos);
      pos += 1;
    } else if (op == 0x00) {
      if (pos + 5 <= data.size()) {
        const auto val = U32LEAt(&data[pos + 1]);
        std::println("  {:4d}  +0x{:04X}  TIME  {}", lineno, pos,
                     static_cast<uint32_t>(val));
      }
      pos += 5;
    } else if (op == 0x01) {
      if (pos + 6 <= data.size()) {
        const auto x = I16LEAt(&data[pos + 1]);
        const auto y = I16LEAt(&data[pos + 3]);
        std::println("  {:4d}  +0x{:04X}  ENEMY  x={} y={} id={}", lineno,
                     pos, static_cast<int16_t>(x), static_cast<int16_t>(y),
                     data[pos + 5]);
      }
      pos += 6;
    } else if (op == 0x02) {
      if (pos + 3 <= data.size()) {
        std::println("  {:4d}  +0x{:04X}  SSP  spd={}", lineno, pos,
                     static_cast<int16_t>(I16LEAt(&data[pos + 1])));
      }
      pos += 3;
    } else if (op == 0x03) {
      std::println("  {:4d}  +0x{:04X}  EFC  type={}", lineno, pos,
                   data[pos + 1]);
      pos += 2;
    } else if (op == 0x04) {
      std::println("  {:4d}  +0x{:04X}  END", lineno, pos);
      pos += 1;
    } else if (op == 0x05) {
      if (pos + 6 <= data.size()) {
        const auto x = I16LEAt(&data[pos + 1]);
        const auto y = I16LEAt(&data[pos + 3]);
        std::println("  {:4d}  +0x{:04X}  BOSS  x={} y={} id={}", lineno,
                     pos, static_cast<int16_t>(x), static_cast<int16_t>(y),
                     data[pos + 5]);
      }
      pos += 6;
    } else if (op == 0x06) {
      std::println("  {:4d}  +0x{:04X}  MWOPEN", lineno, pos);
      pos += 1;
    } else if (op == 0x07) {
      std::println("  {:4d}  +0x{:04X}  MWCLOSE", lineno, pos);
      pos += 1;
    } else if (op == 0x08) { // SCL_MSG
      pos++;
      std::print("  {:4d}  +0x{:04X}  MSG  \"", lineno,
                 static_cast<int>(pos) - 1);
      while (pos < data.size() && data[pos] != 0) {
        const char c = static_cast<char>(data[pos]);
        if (c >= 0x20 && c < 0x7F) {
          std::print("{}", c);
        }
        pos++;
      }
      std::println("\"");
      if (pos < data.size()) {
        pos++;
      }
    } else if (op == 0x09) {
      std::println("  {:4d}  +0x{:04X}  KEY", lineno, pos);
      pos += 1;
    } else if (op == 0x0A) {
      std::println("  {:4d}  +0x{:04X}  NPG", lineno, pos);
      pos += 1;
    } else if (op == 0x0B) {
      std::println("  {:4d}  +0x{:04X}  FACE  id={}", lineno, pos,
                   data[pos + 1]);
      pos += 2;
    } else if (op == 0x0C) {
      std::println("  {:4d}  +0x{:04X}  MUSIC  id={}", lineno, pos,
                   data[pos + 1]);
      pos += 2;
    } else if (op == 0x0E) {
      std::println("  {:4d}  +0x{:04X}  LOADFACE  surf={} file={}", lineno,
                   pos, data[pos + 1], data[pos + 2]);
      pos += 3;
    } else if (op == 0x0F) {
      std::println("  {:4d}  +0x{:04X}  WAITEX  cond={} opt={}", lineno, pos,
                   data[pos + 1], static_cast<uint32_t>(U32LEAt(&data[pos + 2])));
      pos += 6;
    } else if (op == 0x10) {
      std::println("  {:4d}  +0x{:04X}  STAGECLEAR", lineno, pos);
      pos += 1;
    } else if (op == 0x11) {
      std::println("  {:4d}  +0x{:04X}  MAPPALETTE", lineno, pos);
      pos += 1;
    } else if (op == 0x12) {
      std::println("  {:4d}  +0x{:04X}  GAMECLEAR", lineno, pos);
      pos += 1;
    } else if (op == 0x13) {
      std::println("  {:4d}  +0x{:04X}  DELENEMY", lineno, pos);
      pos += 1;
    } else if (op == 0x14) {
      std::println("  {:4d}  +0x{:04X}  ENEMYPALETTE", lineno, pos);
      pos += 1;
    } else if (op == 0x15) {
      std::println("  {:4d}  +0x{:04X}  STAFF  id={}", lineno, pos,
                   data[pos + 1]);
      pos += 2;
    } else if (op == 0x16) {
      std::println("  {:4d}  +0x{:04X}  EXTRACLEAR", lineno, pos);
      pos += 1;
    } else {
      if (name != nullptr) {
        std::println("  {:4d}  +0x{:04X}  {} (unhandled)", lineno, pos, name);
      }
      pos += 1;
    }
    lineno++;
  }
  return true;
}

// ============================================================================
// dump-ecl mode
// ============================================================================

static bool cmd_dump_ecl(const char *in_file, int target_script = -1) {
  auto data = read_file(in_file);
  if (data.empty()) {
    return false;
  }
  if (data.size() < 4) {
    std::println(stderr, "Error: ECL file too small");
    return false;
  }

  const auto script_count = U32LEAt(data.data());
  if (script_count == 0 || script_count > 256) {
    std::println(stderr, "Error: Invalid script count {}",
                 static_cast<uint32_t>(script_count));
    return false;
  }

  std::println("ECL header: {} scripts", static_cast<uint32_t>(script_count));

  for (uint32_t si = 0; si < script_count; si++) {
    const auto offset = U32LEAt(data.data() + 4 + si * 4);
    if (offset >= data.size() || (offset + 9) > data.size()) {
      std::println("  Script {}: offset {:#x} OOB", si,
                   static_cast<uint32_t>(offset));
      continue;
    }
    const uint8_t opcode = data[offset];
    const auto *name = ecl_name(opcode);

    if (target_script >= 0 && static_cast<uint32_t>(target_script) != si) {
      continue;
    }

    if (opcode == 0x00) {
      const auto hp = U32LEAt(&data[offset + 1]);
      const auto score = U32LEAt(&data[offset + 5]);
      std::println("  Script {} @+0x{:04X}: HP={} Score={}", si,
                   static_cast<uint32_t>(offset),
                   static_cast<uint32_t>(hp),
                   static_cast<uint32_t>(score));

      if (target_script >= 0) {
        // Full disassembly
        size_t pos = offset;
        while (pos < data.size()) {
          const uint8_t op = data[pos];
          const auto *n = ecl_name(op);
          const int len = ecl_cmd_len[op];
          if (len <= 0) break;

          std::print("    +0x{:04X}: {:02X} ", pos, op);
          for (int b = 1; b < len && b < 16; b++) {
            std::print("{:02X} ", data[pos + b]);
          }
          for (int b = len; b < 10; b++) {
            std::print("   ");
          }
          std::print("{}", n ? n : "???");

          if (op == 0x00) { // SETUP
            std::print("  HP={} Score={}",
                       static_cast<uint32_t>(U32LEAt(&data[pos + 1])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 5])));
          } else if (op == 0x0C) { // STI
            const uint8_t v = data[pos + 5];
            const auto val = U32LEAt(&data[pos + 6]);
            const auto jmp = U32LEAt(&data[pos + 1]);
            const char *vn = "?";
            if (v == 0x00) vn = "BOSSLEFT";
            else if (v == 0x01) vn = "HP";
            else if (v == 0x02) vn = "TIMER";
            else if (v == 0x03) vn = "BITLEFT";
            std::print("  STI {} jmp=+0x{:04X} val={}", vn,
                       static_cast<uint32_t>(jmp),
                       static_cast<uint32_t>(val));
          } else if (op == 0x02) { // JMP
            std::print("  +0x{:04X}",
                       static_cast<uint32_t>(U32LEAt(&data[pos + 1])));
          } else if (op == 0x07) { // JHPS
            std::print("  hp<{} -> +0x{:04X}",
                       static_cast<uint32_t>(U32LEAt(&data[pos + 1])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 5])));
          } else if (op == 0x06) { // JHPL
            std::print("  hp>{} -> +0x{:04X}",
                       static_cast<uint32_t>(U32LEAt(&data[pos + 1])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 5])));
          } else if (op == 0x08) { // JDIF
            std::print("  E:{:04X} N:{:04X} H:{:04X} L:{:04X}",
                       static_cast<uint32_t>(U32LEAt(&data[pos + 1])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 5])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 9])),
                       static_cast<uint32_t>(U32LEAt(&data[pos + 13])));
          }
          std::println("");

          if (op == 0x01) break; // END
          pos += len;
        }
      } else {
        // Brief: just show STI commands
        size_t pos = offset;
        while (pos < data.size()) {
          const uint8_t op = data[pos];
          const int len = ecl_cmd_len[op];
          if (len <= 0) break;
          if (op == 0x0C && pos + 10 <= data.size()) {
            const uint8_t vect = data[pos + 5];
            const auto val = U32LEAt(&data[pos + 6]);
            const char *vname = "???";
            if (vect == 0x00) vname = "BOSSLEFT";
            else if (vect == 0x01) vname = "HP";
            else if (vect == 0x02) vname = "TIMER";
            else if (vect == 0x03) vname = "BITLEFT";
            std::println("           STI {} val={}", vname,
                         static_cast<uint32_t>(val));
          }
          if (op == 0x01) break;
          pos += len;
        }
      }
    } else {
      std::println("  Script {} @+0x{:04X}: {} (not SETUP)", si,
                   static_cast<uint32_t>(offset),
                   name ? name : "???");
    }
  }
  return true;
}

// ============================================================================
// ecl-time mode — modify STI TIMER values for a specific ECL script
// ============================================================================

static bool cmd_ecl_time(const char *in_file, int script_id, float mult,
                         const char *out_file) {
  if (mult <= 0.0f) {
    std::println(stderr, "Error: Multiplier must be positive");
    return false;
  }

  auto data = read_file(in_file);
  if (data.empty()) return false;
  if (data.size() < 4) {
    std::println(stderr, "Error: ECL file too small");
    return false;
  }

  const auto script_count = U32LEAt(data.data());
  if (script_id < 0 || static_cast<uint32_t>(script_id) >= script_count) {
    std::println(stderr, "Error: Script ID {} out of range (0-{})", script_id,
                 script_count - 1);
    return false;
  }

  const auto offset = U32LEAt(data.data() + 4 + script_id * 4);
  if (offset >= data.size() || data[offset] != 0x00) {
    std::println(stderr, "Error: Script {} does not start with SETUP",
                 script_id);
    return false;
  }

  int modified = 0;
  size_t pos = offset;
  while (pos < data.size()) {
    const uint8_t op = data[pos];
    const int len = ecl_cmd_len[op];
    if (len <= 0) break;
    if (op == 0x0C && pos + 10 <= data.size() && data[pos + 5] == 0x02) {
      const auto old_val = U32LEAt(&data[pos + 6]);
      const auto new_val = static_cast<uint32_t>(
          std::llround(static_cast<double>(old_val) * mult));
      *reinterpret_cast<U32LE *>(&data[pos + 6]) = U32LE(new_val);
      std::println("  +0x{:04X} STI TIMER: {} -> {} (x{:.1f})", pos,
                   static_cast<uint32_t>(old_val), new_val, mult);
      modified++;
    }
    if (op == 0x01) break;
    pos += len;
  }

  std::println("Modified {} STI TIMER(s) in script {}.", modified, script_id);
  return write_file(out_file, data);
}

// ============================================================================
// patch4 mode — write 4 bytes LE at a specific file offset
// ============================================================================

static bool cmd_patch4(const char *in_file, uint32_t offset, uint32_t value,
                       const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty()) return false;
  if (offset + 4 > data.size()) {
    std::println(stderr, "Error: offset {:#x} + 4 > file size {:#x}", offset,
                 data.size());
    return false;
  }
  const auto old_val = U32LEAt(&data[offset]);
  *reinterpret_cast<U32LE *>(&data[offset]) = U32LE(value);
  std::println("  +0x{:04X}: {} -> {}", offset, static_cast<uint32_t>(old_val),
               value);
  return write_file(out_file, data);
}

// ============================================================================
// ecl-boss mode — modify STI HP thresholds and STI TIMER for one script.
// (SETUP HP is handled by the 'ecl' command for all scripts.)
// ============================================================================

static bool cmd_ecl_boss(const char *in_file, int script_id, float hp_mult,
                         float timer_mult, const char *out_file) {
  auto data = read_file(in_file);
  if (data.empty()) return false;
  if (data.size() < 4) {
    std::println(stderr, "Error: ECL file too small");
    return false;
  }

  const auto script_count = U32LEAt(data.data());
  if (script_id < 0 || static_cast<uint32_t>(script_id) >= script_count) {
    std::println(stderr, "Error: Script ID {} out of range (0-{})", script_id,
                 script_count - 1);
    return false;
  }

  const auto offset = U32LEAt(data.data() + 4 + script_id * 4);
  if (offset >= data.size() || data[offset] != 0x00) {
    std::println(stderr, "Error: Script {} does not start with SETUP (0x00)",
                 script_id);
    return false;
  }

  int hp_mods = 0, timer_mods = 0;
  size_t pos = offset;

  while (pos < data.size()) {
    const uint8_t op = data[pos];
    const int len = ecl_cmd_len[op];
    if (len <= 0) break;

    if (op == 0x00 && pos + 9 <= data.size()) { // SETUP
      const auto old_hp = U32LEAt(&data[pos + 1]);
      if (old_hp == 0) break; // death marker, stop
      if (pos != offset) {
        // Phase 2+ SETUP — first one is handled by 'ecl' command
        const auto new_hp = static_cast<uint32_t>(
            std::llround(static_cast<double>(old_hp) * hp_mult));
        *reinterpret_cast<U32LE *>(&data[pos + 1]) = U32LE(new_hp);
        std::println("  +0x{:04X} SETUP  HP: {} -> {} (x{:.1f})", pos,
                     static_cast<uint32_t>(old_hp), new_hp, hp_mult);
        hp_mods++;
      }
    } else if (op == 0x0C && pos + 10 <= data.size()) { // STI
      const uint8_t v = data[pos + 5];
      const auto old_v = U32LEAt(&data[pos + 6]);
      if (v == 0x01) { // HP threshold
        const auto new_v = static_cast<uint32_t>(
            std::llround(static_cast<double>(old_v) * hp_mult));
        *reinterpret_cast<U32LE *>(&data[pos + 6]) = U32LE(new_v);
        std::println("  +0x{:04X} STI HP: {} -> {} (x{:.1f})", pos,
                     static_cast<uint32_t>(old_v), new_v, hp_mult);
        hp_mods++;
      } else if (v == 0x02) { // TIMER
        const auto new_v = static_cast<uint32_t>(
            std::llround(static_cast<double>(old_v) * timer_mult));
        *reinterpret_cast<U32LE *>(&data[pos + 6]) = U32LE(new_v);
        std::println("  +0x{:04X} STI TIMER: {} -> {} (x{:.1f})", pos,
                     static_cast<uint32_t>(old_v), new_v, timer_mult);
        timer_mods++;
      }
    }

    if (op == 0x01) break; // END
    pos += len;
  }

  std::println("Script {}: {} HP/threshold mods, {} TIMER mods", script_id,
               hp_mods, timer_mods);
  return write_file(out_file, data);
}

// ============================================================================
// Extract-music mode — extract MIDI + title + comment from MUSIC.DAT/ENEMY.DAT
// ============================================================================

static bool cmd_extract_music(const char *data_dir, const char *out_dir) {
  namespace fs = std::filesystem;

  const auto music_path = fs::path(data_dir) / "MUSIC.DAT";
  const auto enemy_path = fs::path(data_dir) / "ENEMY.DAT";

  auto music_reader = FilStartR(music_path.string().c_str());
  if (!music_reader) {
    std::println(stderr, "Error: Cannot open '{}'", music_path.string());
    return false;
  }

  auto enemy_reader = FilStartR(enemy_path.string().c_str());
  if (!enemy_reader) {
    std::println(stderr, "Error: Cannot open '{}'", enemy_path.string());
    return false;
  }

  const auto music_count = static_cast<int>(music_reader.info.size());
  std::println("Extracting {} music tracks from '{}' to '{}'...", music_count,
               data_dir, out_dir);

  std::error_code ec;
  for (int i = 0; i < music_count; i++) {
    const auto track_dir =
        fs::path(out_dir) / std::format("track_{:02}", i);
    fs::create_directories(track_dir, ec);
    if (ec) {
      std::println(stderr, "Error: Cannot create '{}'", track_dir.string());
      return false;
    }

    auto midi_data = music_reader.MemExpand(i);
    if (!midi_data) {
      std::println(stderr, "Error: Failed to extract MIDI entry {}", i);
      return false;
    }

    const std::vector<uint8_t> midi_vec(midi_data.get(),
                                        midi_data.get() + midi_data.size());
    if (!write_file((track_dir / "midi.mid").string().c_str(), midi_vec)) {
      return false;
    }

    const auto title_raw = extract_smf_title(midi_vec);
    const auto title_utf8 = gbk_to_utf8(title_raw);
    std::string_view title_trimmed = title_utf8;
    while (!title_trimmed.empty()) {
      if (title_trimmed.starts_with(" ")) {
        title_trimmed.remove_prefix(1);
      } else if (title_trimmed.starts_with("\u3000")) {
        title_trimmed.remove_prefix(3);
      } else {
        break;
      }
    }
    title_trimmed =
        title_trimmed.substr(0, title_trimmed.find_last_not_of(" ") + 1);
    if (!write_text_file(track_dir / "title.txt", title_trimmed)) {
      return false;
    }

    const int comment_index = 27 + i;
    if (comment_index < static_cast<int>(enemy_reader.info.size())) {
      auto comment_data = enemy_reader.MemExpand(comment_index);
      if (comment_data) {
        const std::vector<uint8_t> comment_vec(
            comment_data.get(), comment_data.get() + comment_data.size());
        auto comment_utf8 = parse_music_comment(comment_vec);
        if (!write_text_file(track_dir / "comment.txt", comment_utf8)) {
          return false;
        }
      }
    }

    std::println("  track_{:02}  MIDI {} bytes  title=[{}]", i, midi_data.size(),
                 title_trimmed);
  }

  std::println("Done.");
  return true;
}

// ============================================================================
// Pack-music mode — pack unified MUSIC.PAK from extracted track directories
// ============================================================================

static bool cmd_pack_music(const char *in_dir, const char *out_packfile) {
  namespace fs = std::filesystem;

  std::error_code ec;
  if (!fs::is_directory(in_dir, ec)) {
    std::println(stderr, "Error: '{}' is not a directory", in_dir);
    return false;
  }

  std::vector<fs::path> track_dirs;
  for (const auto &entry : fs::directory_iterator(in_dir)) {
    if (entry.is_directory() &&
        entry.path().filename().string().starts_with("track_")) {
      track_dirs.push_back(entry.path());
    }
  }
  std::sort(track_dirs.begin(), track_dirs.end());

  if (track_dirs.empty()) {
    std::println(stderr, "Error: No track_* directories found in '{}'", in_dir);
    return false;
  }

  std::println("Packing {} tracks to '{}'...", track_dirs.size(),
               out_packfile);

  PACKFILE_WRITE writer;
  std::vector<std::vector<uint8_t>> buffers;
  buffers.reserve(track_dirs.size());

  for (const auto &track_dir : track_dirs) {
    const auto title = read_text_file(track_dir / "title.txt");
    const auto comment = read_text_file(track_dir / "comment.txt");
    auto midi = read_file((track_dir / "midi.mid").string().c_str());
    if (midi.empty()) {
      std::println(stderr, "Error: Missing midi.mid in '{}'",
                   track_dir.string());
      return false;
    }

    std::vector<uint8_t> entry;
    const auto push_u32_le = [&](uint32_t val) {
      entry.push_back(static_cast<uint8_t>(val));
      entry.push_back(static_cast<uint8_t>(val >> 8));
      entry.push_back(static_cast<uint8_t>(val >> 16));
      entry.push_back(static_cast<uint8_t>(val >> 24));
    };

    push_u32_le(static_cast<uint32_t>(title.size()));
    entry.insert(entry.end(), title.begin(), title.end());
    push_u32_le(static_cast<uint32_t>(comment.size()));
    entry.insert(entry.end(), comment.begin(), comment.end());
    entry.insert(entry.end(), midi.begin(), midi.end());

    std::println("  {}  title=[{}]  comment={} B  MIDI={} B",
                 track_dir.filename().string(), title, comment.size(),
                 midi.size());

    buffers.emplace_back(std::move(entry));
  }

  for (auto &buf : buffers) {
    writer.files.emplace_back(buf.data(), buf.size());
  }

  if (!writer.Write(out_packfile)) {
    std::println(stderr, "Error: Failed to write pack file '{}'", out_packfile);
    return false;
  }

  std::println("Done.");
  return true;
}

// ============================================================================
// Strip mode — remove legacy script and comment entries from an old
// 48-entry ENEMY.DAT. After conversion to 13-entry MAP.PAK, no entries
// need stripping.
// ============================================================================

static bool cmd_strip(const char *in_packfile, const char *out_packfile) {
  auto reader = FilStartR(in_packfile);
  if (!reader) {
    std::println(stderr, "Error: Cannot open pack file '{}'", in_packfile);
    return false;
  }

  // Entry indices to replace with placeholders (only valid for old 48-entry ENEMY.DAT)
  const std::unordered_set<int> strip_entries = [] {
    std::unordered_set<int> s = {
        0, 1, 2, 3, 4, 5,    // Stage 1-6 ECL (embedded)
        6, 7, 8, 9, 10, 11,  // Stage 1-6 SCL (embedded)
        24,                   // Extra stage ECL (embedded)
        25,                   // Extra stage SCL (embedded)
        47                    // Ending SCL (embedded)
    };
    for (int i = 27; i <= 46; i++)
      s.insert(i); // Music room comments (migrated to MUSIC.PAK)
    return s;
  }();

  const auto n = static_cast<int>(reader.info.size());
  std::println("Stripping entries from '{}'...", in_packfile);
  std::println("  {} total entries, {} entries will be stripped", n,
               strip_entries.size());

  // Storage for entry data (writer holds non-owning views)
  std::vector<std::vector<uint8_t>> storage;
  storage.reserve(n);

  PACKFILE_WRITE writer;
  size_t skipped_bytes = 0;

  for (int i = 0; i < n; i++) {
    auto data = reader.MemExpand(i);
    if (!data) {
      std::println(stderr, "Error: Failed to decompress entry {}", i);
      return false;
    }

    if (strip_entries.count(i)) {
      skipped_bytes += data.size();
      // Replace with zero-byte placeholder
      storage.push_back({});
      writer.files.emplace_back(
          storage.back().data(), storage.back().size());
    } else {
      // Keep original data
      auto &buf = storage.emplace_back(data.get(), data.get() + data.size());
      writer.files.emplace_back(buf.data(), buf.size());
    }
  }

  if (!writer.Write(out_packfile)) {
    std::println(stderr, "Error: Failed to write stripped pack file '{}'",
                 out_packfile);
    return false;
  }

  std::println("Done. {} bytes of data stripped.", skipped_bytes);
  return true;
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  const std::string_view mode = argv[1];

  if (mode == "extract") {
    if (argc != 4) {
      std::println(stderr, "Usage: pack_tool extract <packfile> <out_dir>");
      return 1;
    }
    return cmd_extract(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "pack") {
    if (argc != 4) {
      std::println(stderr, "Usage: pack_tool pack <in_dir> <packfile>");
      return 1;
    }
    return cmd_pack(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "strip") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: pack_tool strip <in_packfile> <out_packfile>");
      return 1;
    }
    return cmd_strip(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "extract-music") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: pack_tool extract-music <data_dir> <out_dir>");
      return 1;
    }
    return cmd_extract_music(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "pack-music") {
    if (argc != 4) {
      std::println(stderr,
                   "Usage: pack_tool pack-music <in_dir> <out_packfile>");
      return 1;
    }
    return cmd_pack_music(argv[2], argv[3]) ? 0 : 1;
  }

  if (mode == "scl") {
    if (argc != 5) {
      std::println(stderr,
                   "Usage: pack_tool scl <in_file> <multiplier> <out_file>");
      return 1;
    }
    const float mult = std::strtof(argv[3], nullptr);
    return cmd_scl(argv[2], mult, argv[4]) ? 0 : 1;
  }

  if (mode == "ecl") {
    if (argc != 5) {
      std::println(stderr,
                   "Usage: pack_tool ecl <in_file> <multiplier> <out_file>");
      return 1;
    }
    const float mult = std::strtof(argv[3], nullptr);
    return cmd_ecl(argv[2], mult, argv[4]) ? 0 : 1;
  }

  if (mode == "ecl-time") {
    if (argc != 6) {
      std::println(stderr,
                   "Usage: pack_tool ecl-time <in_file> <script_id> "
                   "<multiplier> <out_file>");
      return 1;
    }
    const int script_id = std::atoi(argv[3]);
    const float mult = std::strtof(argv[4], nullptr);
    return cmd_ecl_time(argv[2], script_id, mult, argv[5]) ? 0 : 1;
  }

  if (mode == "ecl-boss") {
    if (argc != 7) {
      std::println(stderr,
                   "Usage: pack_tool ecl-boss <in_file> <script_id> "
                   "<hp_mult> <timer_mult> <out_file>");
      return 1;
    }
    const int script_id = std::atoi(argv[3]);
    const float hp_mult = std::strtof(argv[4], nullptr);
    const float timer_mult = std::strtof(argv[5], nullptr);
    return cmd_ecl_boss(argv[2], script_id, hp_mult, timer_mult, argv[6]) ? 0
                                                                          : 1;
  }

  if (mode == "dump-scl") {
    if (argc != 3) {
      std::println(stderr, "Usage: pack_tool dump-scl <in_file>");
      return 1;
    }
    return cmd_dump_scl(argv[2]) ? 0 : 1;
  }

  if (mode == "dump-ecl") {
    if (argc < 3 || argc > 4) {
      std::println(stderr, "Usage: pack_tool dump-ecl <in_file> [script_id]");
      return 1;
    }
    const int sid = (argc >= 4) ? std::atoi(argv[3]) : -1;
    return cmd_dump_ecl(argv[2], sid) ? 0 : 1;
  }

  if (mode == "patch4") {
    if (argc != 6) {
      std::println(stderr,
                   "Usage: pack_tool patch4 <in_file> <offset_hex> "
                   "<value_hex> <out_file>");
      return 1;
    }
    const auto offset =
        static_cast<uint32_t>(std::strtoul(argv[3], nullptr, 0));
    const auto value =
        static_cast<uint32_t>(std::strtoul(argv[4], nullptr, 0));
    return cmd_patch4(argv[2], offset, value, argv[5]) ? 0 : 1;
  }

  std::println(stderr, "Unknown mode: '{}'", mode);
  print_usage();
  return 1;
}
