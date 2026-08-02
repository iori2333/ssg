///
/// Time interface
///
#pragma once

#include <chrono>
#include <cstdint>

namespace util {

using LocalTimePoint = std::chrono::local_time<std::chrono::seconds>;
using UtcTimePoint = std::chrono::sys_time<std::chrono::seconds>;

// Returns some kind of steady system tick value in milliseconds.
inline int64_t SteadyTicksMs() {
  const auto now = (std::chrono::steady_clock::now().time_since_epoch());
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

// Returns the current UTC time.
inline UtcTimePoint UtcTime() {
  return std::chrono::floor<std::chrono::seconds>(
      std::chrono::system_clock::now());
}

// Converts a Unix timestamp in seconds to UTC.
inline UtcTimePoint UtcTime(int64_t unix_seconds) {
  return UtcTimePoint{std::chrono::seconds{unix_seconds}};
}

// Converts a UTC time point to local time.
inline LocalTimePoint LocalTime(UtcTimePoint utc) {
  return std::chrono::zoned_time{std::chrono::current_zone(), utc}
      .get_local_time();
}

// Returns the current local system time.
inline LocalTimePoint LocalTime() { return LocalTime(UtcTime()); }

} // namespace util
