///
/// File I/O
///

#include <fstream>
#include <limits>

#include "file.h"

BYTE_BUFFER_OWNED File_Load(const std::filesystem::path &path) {
  std::ifstream stream(path, std::ios::binary | std::ios::ate);
  if (!stream) {
    return {};
  }

  const auto end = stream.tellg();
  if (end <= 0 ||
      static_cast<uintmax_t>(end) > std::numeric_limits<size_t>::max() ||
      static_cast<uintmax_t>(end) >
          std::numeric_limits<std::streamsize>::max()) {
    return {};
  }
  BYTE_BUFFER_OWNED data(static_cast<size_t>(end));
  stream.seekg(0);
  stream.read(reinterpret_cast<char *>(data.get()),
              static_cast<std::streamsize>(data.size()));
  return stream ? std::move(data) : BYTE_BUFFER_OWNED{};
}

bool File_Save(const std::filesystem::path &path,
               std::span<const uint8_t> data) {
  if (static_cast<uintmax_t>(data.size()) >
      static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
    return false;
  }
  std::ofstream stream(path, std::ios::binary | std::ios::trunc);
  if (!stream) {
    return false;
  }
  stream.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
  stream.close();
  return static_cast<bool>(stream);
}
