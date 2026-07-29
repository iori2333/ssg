///
/// Debug output functions
///

#include <format>
#include <fstream>

#include "debug.h"
#include "time.h"
// Global variables
constexpr auto ErrorOut = "ErrLOG_UTF8.TXT";
static bool ErrorActive = false;

void DebugLog(std::string_view prefix, std::string_view s) {
  if (!ErrorActive) {
    return;
  }
  std::ofstream file(ErrorOut, std::ios::binary | std::ios::app);
  if (!file) {
    return;
  }
  file << prefix << s << '\n';
}

extern void DebugSetup() {
  const auto tm = Time_NowLocal();
  auto str =
      std::format("[{:02}/{:02}/{:02}][{:02}:{:02}:{:02}]", tm.month, tm.day,
                  (tm.year % 100), tm.hour, tm.minute, tm.second);
  ErrorActive = true;
  DebugLog("", str);
}

extern void DebugCleanup(void) { ErrorActive = false; }

void DebugLog(std::string_view s) { return DebugLog("", s); }

extern void DebugOut(std::string_view s) { return DebugLog("Error : ", s); }
