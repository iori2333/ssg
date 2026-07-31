///
/// Time interface (std::chrono implementation)
///

#include <cassert>
#include <chrono>
#include <ctime>

#include "time.h"

namespace util {

uint32_t SteadyTicksMs() {
  const auto now = (std::chrono::steady_clock::now().time_since_epoch());
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

TimeOfDay LocalTime() {
  const auto ctime = std::time(nullptr);
  std::tm tm{};
#ifdef _MSC_VER
  localtime_s(&tm, &ctime);
#else
  tm = *std::localtime(&ctime);
#endif

  assert(tm.tm_year >= 0);
  assert(tm.tm_mon >= 0);
  assert(tm.tm_mday >= 0);
  assert(tm.tm_hour >= 0);
  assert(tm.tm_min >= 0);
  assert(tm.tm_sec >= 0);

  return TimeOfDay{
      .year = static_cast<uint32_t>(1900 + tm.tm_year),
      .month = static_cast<uint8_t>(1 + tm.tm_mon),
      .day = static_cast<uint8_t>(tm.tm_mday),
      .hour = static_cast<uint8_t>(tm.tm_hour),
      .minute = static_cast<uint8_t>(tm.tm_min),
      .second = static_cast<uint8_t>(tm.tm_sec),
  };
}

} // namespace util
