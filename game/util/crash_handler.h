/// Debug-only fatal error reporting.
#pragma once

#include <cstddef>
#include <string_view>

namespace crash {

void Install();
void Uninstall();
void Report(std::string_view reason, std::size_t frames_to_skip = 0) noexcept;

namespace platform {

void Install();
void Uninstall();

} // namespace platform

} // namespace crash
