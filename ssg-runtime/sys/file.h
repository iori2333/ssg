///
/// File I/O
///

#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

[[nodiscard]] std::vector<uint8_t> LoadFile(const std::filesystem::path &path);
[[nodiscard]] bool SaveFile(const std::filesystem::path &path,
                            std::span<const uint8_t> data);
