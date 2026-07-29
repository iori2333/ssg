///
/// File I/O
///

#pragma once

#include <filesystem>
#include <span>

#include "buffer.h"

[[nodiscard]] BYTE_BUFFER_OWNED File_Load(const std::filesystem::path &path);
[[nodiscard]] bool File_Save(const std::filesystem::path &path,
                             std::span<const uint8_t> data);
