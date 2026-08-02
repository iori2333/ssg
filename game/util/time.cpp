///
/// Time interface (std::chrono implementation)
///

#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>

#include "time_api.h"

namespace util {

int64_t SteadyTicksMs() {
  const auto now = (std::chrono::steady_clock::now().time_since_epoch());
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

TimeOfDay LocalTime() {
  const auto ctime = std::time(nullptr);
  std::tm tm{};
#ifdef _MSC_VER
  if (localtime_s(&tm, &ctime) != 0) {
    return {};
  }
#else
  const auto *local = std::localtime(&ctime);
  if (local == nullptr) {
    return {};
  }
  tm = *local;
#endif

  assert(tm.tm_year >= 0);
  assert(tm.tm_mon >= 0);
  assert(tm.tm_mday >= 0);
  assert(tm.tm_hour >= 0);
  assert(tm.tm_min >= 0);
  assert(tm.tm_sec >= 0);

  return TimeOfDay{
      .year = 1900 + tm.tm_year,
      .month = 1 + tm.tm_mon,
      .day = tm.tm_mday,
      .hour = tm.tm_hour,
      .minute = tm.tm_min,
      .second = tm.tm_sec,
  };
}

} // namespace util
