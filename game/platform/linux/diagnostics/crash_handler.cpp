/// Linux fatal signal entry point.

#include <algorithm>
#include <array>
#include <charconv>
#include <csignal>
#include <cstddef>
#include <string_view>

#include "util/crash_handler.h"

namespace crash::platform {
namespace {

struct SignalRegistration {
  int signal;
  struct sigaction previous{};
  bool installed = false;
};

std::array Registrations = {
    SignalRegistration{SIGABRT}, SignalRegistration{SIGBUS},
    SignalRegistration{SIGFPE},  SignalRegistration{SIGILL},
    SignalRegistration{SIGSEGV},
};

void HandleFatalSignal(int signal, siginfo_t *info, void *) {
  static_cast<void>(info);
  constexpr std::string_view prefix = "Fatal signal ";
  std::array<char, 32> reason{};
  std::ranges::copy(prefix, reason.begin());
  const auto result =
      std::to_chars(reason.data() + prefix.size(), reason.end(), signal);
  const auto reason_size = static_cast<std::size_t>(result.ptr - reason.data());
  Report(std::string_view(reason.data(), reason_size), 1);
  std::signal(signal, SIG_DFL);
  std::raise(signal);
}

} // namespace

void Install() {
  struct sigaction action{};
  sigemptyset(&action.sa_mask);
  action.sa_sigaction = HandleFatalSignal;
  action.sa_flags = SA_SIGINFO | SA_RESETHAND;

  for (auto &registration : Registrations) {
    registration.installed =
        sigaction(registration.signal, &action, &registration.previous) == 0;
  }
}

void Uninstall() {
  for (auto &registration : Registrations) {
    if (registration.installed) {
      sigaction(registration.signal, &registration.previous, nullptr);
      registration.installed = false;
    }
  }
}

} // namespace crash::platform
