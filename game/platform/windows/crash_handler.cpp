/// Windows fatal exception entry point.

#include <cstdint>
#include <format>

#include <windows.h>

#include "sys/crash_handler.h"

namespace crash::platform {
namespace {

LPTOP_LEVEL_EXCEPTION_FILTER PreviousFilter;

LONG WINAPI HandleUnhandledException(EXCEPTION_POINTERS *exception) noexcept {
  try {
    const auto code =
        static_cast<uint32_t>(exception->ExceptionRecord->ExceptionCode);
    const auto address = exception->ExceptionRecord->ExceptionAddress;
    Report(std::format("Unhandled Windows exception 0x{:08X} at {}", code,
                       static_cast<const void *>(address)),
           1);
  } catch (...) {
    Report("Unhandled Windows exception", 1);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

} // namespace

void Install() {
  PreviousFilter = SetUnhandledExceptionFilter(HandleUnhandledException);
}

void Uninstall() { SetUnhandledExceptionFilter(PreviousFilter); }

} // namespace crash::platform
