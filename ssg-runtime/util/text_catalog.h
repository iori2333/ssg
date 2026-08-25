///
/// On-disk text catalog format contract, shared by the generator
/// (ssg-scripts/tools/script_tool.cpp) and the runtime parser
/// (ssg/i18n/localization.cpp).
///

#pragma once

#include <array>
#include <cstdint>

namespace i18n {

// Fixed 4-byte magic: "SSTX".
inline constexpr std::array<uint8_t, 4> kCatalogMagic = {'S', 'S', 'T', 'X'};

// Bump whenever the catalog layout changes; both producer and consumer must
// agree.
inline constexpr uint32_t kCatalogVersion = 2;

} // namespace i18n