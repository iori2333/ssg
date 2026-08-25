///
/// Single source of truth for ECL opcode encoded sizes (in bytes, including
/// the opcode byte). 0 marks an undefined opcode.
///
/// Shared by the runtime decoder (`ssg/enemy/ecl/ecl_program.cpp`) and the
/// script tool (`ssg-scripts/tools/script_tool.cpp`) so the two length tables
/// cannot drift apart.
///

#pragma once

#include <array>
#include <cstdint>

namespace ecl {

inline constexpr std::array<uint8_t, 0x100> kEclOpcodeSizes = {
    9, 1, 5, 7, 5, 1, 9, 9, 17, 5, 9, 9,
    10, 2, 0, 0, 3, 3, 3, 4, 12, 9, 9, 5,
    5, 7, 3, 3, 3, 4, 7, 2, 2, 2, 1, 1,
    5, 5, 5, 5, 1, 1, 1, 1, 1, 1, 3, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 2, 5, 2, 3, 3, 3, 3,
    3, 3, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1,
    2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 5, 5, 5, 3, 3, 2, 2, 5, 5, 2,
    2, 5, 1, 1, 5, 1, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2,
    3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 3, 2, 2, 2, 6, 7, 5, 2,
    2, 2, 2, 5, 6, 2, 6, 1, 3, 6, 3, 3,
    3, 3, 6, 2, 3, 6, 5, 5, 2, 2, 5, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0,
};

} // namespace ecl