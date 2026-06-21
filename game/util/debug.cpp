///
/// Debug output functions
///

#include <SDL3/SDL_iostream.h>

#include <format>

#include "util/debug.h"
#include "sys/file.h"
#include "util/time.h"
// Global variables
constexpr auto ErrorOut = "ErrLOG_UTF8.TXT";
static bool ErrorActive = false;

void DebugLog(std::string_view prefix, std::string_view s) {
  if (!ErrorActive) {
    return;
  }
  auto *f = SDL_IOFromFile(ErrorOut, "ab");
  if (!f) {
    return;
  }
  SDL_WriteIO(f, prefix.data(), prefix.size());
  SDL_WriteIO(f, s.data(), s.size());
  SDL_WriteIO(f, "\n", 1);
  SDL_CloseIO(f);
}

extern void DebugSetup() {
  const auto tm = Time_NowLocal();
  auto str = std::format("[{:02}/{:02}/{:02}][{:02}:{:02}:{:02}]", tm.month,
                         tm.day, (tm.year % 100), tm.hour, tm.minute, tm.second);
  ErrorActive = true;
  DebugLog("", str);
}

extern void DebugCleanup(void) { ErrorActive = false; }

void DebugLog(std::string_view s) { return DebugLog("", s); }

extern void DebugOut(std::string_view s) { return DebugLog("Error : ", s); }
