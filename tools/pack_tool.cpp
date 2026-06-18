///
/// pack_tool - GIAN07 ENEMY.DAT pack file modification tool
///
/// Usage:
///   pack_tool extract <packfile> <out_dir>
///   pack_tool pack <in_dir> <packfile>
///   pack_tool scl <in_file> <multiplier> <out_file>
///   pack_tool ecl <in_file> <multiplier> <out_file>
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

#include "GIAN07/core/lz_uty.h"
#include "game/endian.h"

namespace fs = std::filesystem;

namespace fs = std::filesystem;

// ============================================================================
// Helpers
// ============================================================================

static void print_usage() {
  std::println(stderr, R"(pack_tool - GIAN07 ENEMY.DAT pack file tool

Usage:
  pack_tool extract <packfile> <out_dir>
      Extract all entries from a PBG pack file to out_dir/NNN.bin

  pack_tool pack <in_dir> <packfile>
      Repack all NNN.bin files from in_dir into a PBG pack file

  pack_tool scl <in_file> <multiplier> <out_file>
      Multiply the last boss SCL_TIME frame value by multiplier
      (finds last SCL_BOSSDEAD, walks back to preceding SCL_TIME)

  pack_tool ecl <in_file> <multiplier> <out_file>
      Multiply all ECL SETUP HP values by multiplier
      (parses ECL header, modifies SETUP HP in every script)
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

  // Find SCL_BOSSDEAD (0x0D) preceded by SCL_TIME (0x00).
  // SCL_TIME is 5 bytes, but other SCL commands (ENEMY=6, EFC=2, SSP=3, etc.)
  // may appear between SCL_TIME and SCL_BOSSDEAD. Scan each 0x0D and check
  // multiple possible SCL_TIME offsets back. Pick the last valid match.
  // Validate frame value: must be between 60 and 10,000,000 (~1s to ~44h).
  constexpr uint32_t MIN_FRAME = 60;
  constexpr uint32_t MAX_FRAME = 10000000u;

  int best_pos = -1;
  uint32_t best_val = 0;

  for (size_t i = 5; i < data.size(); i++) {
    if (data[i] != 0x0D) {
      continue;
    }
    // Try various known SCL command gap sizes
    for (int ofs : {5, 7, 8, 9, 12}) {
      if (static_cast<int>(i) < ofs) {
        continue;
      }
      const int tp = static_cast<int>(i) - ofs;
      if (data[tp] == 0x00) {
        const auto val = U32LEAt(&data[tp + 1]);
        if (val >= MIN_FRAME && val <= MAX_FRAME) {
          best_pos = tp;
          best_val = val;
          break; // found for this BOSSDEAD, skip other offsets
        }
      }
    }
  }

  if (best_pos < 0) {
    std::println(stderr,
                 "Error: No SCL_TIME+SCL_BOSSDEAD sequence found "
                 "- this may not be an SCL file");
    return false;
  }

  const auto new_val = static_cast<uint32_t>(
      std::llround(static_cast<double>(best_val) * mult));
  *reinterpret_cast<U32LE *>(&data[best_pos + 1]) = U32LE(new_val);

  std::println("Modified SCL_TIME at +0x{:04X}: {} -> {} (x{:.1f})",
               best_pos, best_val, new_val, mult);

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

  std::println(stderr, "Unknown mode: '{}'", mode);
  print_usage();
  return 1;
}
