///
/// Common paths and path manipulation
///

#pragma once

#include <string_view>

// Returns the directory containing the game's data. Guaranteed to end with the
// native directory separator. Can be the empty string on platforms with no
// concept of a base data directory.
std::string_view PathForData(void);
