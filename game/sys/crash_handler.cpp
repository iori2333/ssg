/// Debug-only fatal error reporting.

#include <atomic>
#include <cstdlib>
#include <exception>
#include <stacktrace>
#include <string>

#include "crash_handler.h"

#include "sys/log.h"

namespace crash {
namespace {

std::terminate_handler PreviousTerminate;
std::atomic_flag Reporting = ATOMIC_FLAG_INIT;
bool Installed = false;

[[noreturn]] void Terminate() noexcept {
  try {
    const auto exception = std::current_exception();
    if (exception) {
      std::rethrow_exception(exception);
    }
    Report("std::terminate called", 1);
  } catch (const std::exception &error) {
    Report(error.what(), 1);
  } catch (...) {
    Report("Unhandled non-standard C++ exception", 1);
  }

  if ((PreviousTerminate != nullptr) && PreviousTerminate != Terminate) {
    PreviousTerminate();
  }
  std::abort();
}

} // namespace

void Report(std::string_view reason, std::size_t frames_to_skip) noexcept {
  if (Reporting.test_and_set()) {
    return;
  }

  try {
    logging::Critical(logging::Channel::Crash, "Fatal error: {}", reason);
    logging::Write(
        logging::Level::Critical, logging::Channel::Crash,
        std::to_string(std::stacktrace::current(frames_to_skip + 1)));
    logging::Flush();
  } catch (...) {
    logging::Write(logging::Level::Critical, logging::Channel::Crash,
                   "Fatal error occurred while generating the stack trace");
    logging::Flush();
  }
}

void Install() {
  if (Installed) {
    return;
  }
  Reporting.clear();
  PreviousTerminate = std::set_terminate(Terminate);
  platform::Install();
  Installed = true;
}

void Uninstall() {
  if (!Installed) {
    return;
  }
  platform::Uninstall();
  if (std::get_terminate() == Terminate) {
    std::set_terminate(PreviousTerminate);
  }
  Installed = false;
}

} // namespace crash
