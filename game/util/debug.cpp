///
/// Debug output functions
///

#include <format>

#include <SDL3/SDL_iostream.h>

#include "debug.h"
#include "time.h"

#include "sys/file.h"
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
  auto str =
      std::format("[{:02}/{:02}/{:02}][{:02}:{:02}:{:02}]", tm.month, tm.day,
                  (tm.year % 100), tm.hour, tm.minute, tm.second);
  ErrorActive = true;
  DebugLog("", str);
  DebugInstallCrashHandler();
}

extern void DebugCleanup(void) { ErrorActive = false; }

void DebugLog(std::string_view s) { return DebugLog("", s); }

extern void DebugOut(std::string_view s) { return DebugLog("Error : ", s); }

// ============================================================
// Crash diagnostics (DEBUG builds only)
// ============================================================
// On a fatal signal / SEH exception / std::terminate we capture a
// std::stacktrace and write it to the error log and stderr so the failing
// call site is obvious without an external debugger.  The handlers run in
// a signal / exception context, so they are *not* strictly async-signal-
// safe (std::stacktrace allocates) — acceptable for a best-effort post-
// mortem on an already-dying process.
#ifdef PBG_DEBUG
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <stacktrace>
#include <string>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
void DumpStacktrace(const char *reason) {
  // Ensure the log file receives output even if the crash precedes the
  // completion of DebugSetup().
  ErrorActive = true;
  const auto st = std::stacktrace::current(0, 64);
  const std::string rendered = std::to_string(st);

  std::fputs("\n===== CRASH: ", stderr);
  std::fputs(reason, stderr);
  std::fputs(" =====\n", stderr);
  std::fputs(rendered.c_str(), stderr);
  std::fputc('\n', stderr);
  std::fflush(stderr);

  DebugLog("===== CRASH =====", reason);
  for (const auto &entry : st) {
    DebugLog("  ", entry.description());
  }
}

[[noreturn]] void CrashExit() { std::_Exit(EXIT_FAILURE); }

extern "C" void SignalHandler(int sig) {
  const char *r = (sig == SIGABRT) ? "abort / debug assertion"
                 : (sig == SIGSEGV) ? "segfault"
                 : (sig == SIGILL) ? "illegal instruction"
                 : (sig == SIGFPE) ? "arithmetic"
                                   : "signal";
  DumpStacktrace(r);
  CrashExit();
}
} // namespace

#ifdef WIN32
LONG WINAPI VectoredCrashHandler(PEXCEPTION_POINTERS ep) {
  // Only dump for fatal hardware exceptions; let the CRT / debugger keep
  // everything else (e.g. C++ exceptions handled by the game).
  const DWORD code = ep ? ep->ExceptionRecord->ExceptionCode : 0;
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
  case EXCEPTION_ILLEGAL_INSTRUCTION:
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
  case EXCEPTION_STACK_OVERFLOW:
  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
  case EXCEPTION_PRIV_INSTRUCTION:
  case EXCEPTION_IN_PAGE_ERROR:
    DumpStacktrace("SEH exception");
    break;
  default:
    break;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

extern void DebugInstallCrashHandler() {
  std::signal(SIGABRT, SignalHandler);
  std::signal(SIGSEGV, SignalHandler);
  std::signal(SIGILL, SignalHandler);
  std::signal(SIGFPE, SignalHandler);
  std::set_terminate([] {
    DumpStacktrace("std::terminate");
    CrashExit();
  });
#ifdef WIN32
  AddVectoredExceptionHandler(0 /* last-chance */, VectoredCrashHandler);
#endif
}
#else
extern void DebugInstallCrashHandler() {}
#endif
