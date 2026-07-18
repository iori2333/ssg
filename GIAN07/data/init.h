///
/// Data initialization entry point
///
#pragma once

#include <optional>
#include <string>

/// Returns nullopt on success, or an error message string on failure.
std::optional<std::string> DataInit();
void DataCleanup();
