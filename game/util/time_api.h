///
/// Platform-specific time interface
///
#pragma once

#include <cstdint>

namespace util {

struct TimeOfDay {
  uint32_t year;
  uint8_t month; // 1-based
  uint8_t day;   // 1-based
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

// Returns some kind of steady system tick value in milliseconds.
uint32_t SteadyTicksMs();

// Returns the current local system time.
TimeOfDay LocalTime();

} // namespace util
