///
/// Platform-specific time interface
///
#pragma once

#include <cstdint>

namespace util {

struct TimeOfDay {
  int year;
  int month; // 1-based
  int day;   // 1-based
  int hour;
  int minute;
  int second;
};

// Returns some kind of steady system tick value in milliseconds.
int64_t SteadyTicksMs();

// Returns the current local system time.
TimeOfDay LocalTime();

} // namespace util
