/// Linux fatal signal entry point.

#include <array>
#include <csignal>
#include <cstddef>

#include <unistd.h>

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
  constexpr char prefix[] = "Fatal signal ";
  std::array<char, 32> message{};
  std::size_t size = sizeof(prefix) - 1;
  for (std::size_t i = 0; i < size; i++) {
    message[i] = prefix[i];
  }

  std::array<char, 10> digits{};
  std::size_t digit_count = 0;
  auto value = static_cast<unsigned int>(signal);
  do {
    digits[digit_count++] = static_cast<char>('0' + (value % 10));
    value /= 10;
  } while (value != 0);
  while (digit_count != 0) {
    message[size++] = digits[--digit_count];
  }
  message[size++] = '\n';
  static_cast<void>(write(STDERR_FILENO, message.data(), size));

  kill(getpid(), signal);
  _exit(128 + signal);
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
