/// Application-wide structured logging.
#pragma once

#include <cstdint>
#include <format>
#include <string_view>
#include <utility>

namespace logging {

enum class Level : uint8_t {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Critical,
  Off,
};

enum class Channel : uint8_t {
  Application,
  Config,
  Data,
  I18n,
  GameFlow,
  Stage,
  Gameplay,
  Record,
  Audio,
  Music,
  Graphics,
  Input,
  Ui,
  Platform,
  Crash,
  Sdl,
};

// SSG_LOG_LEVEL sets the default threshold. SSG_LOG applies comma-separated
// channel overrides such as "stage=trace,music=debug".
void Initialize(std::string_view base_directory, std::string_view application,
                std::string_view version);
void Shutdown();
void Flush() noexcept;

[[nodiscard]] bool Enabled(Level level, Channel channel) noexcept;
void Write(Level level, Channel channel, std::string_view message) noexcept;
void SdlError(Channel channel, std::string_view context) noexcept;

template <typename... Args>
void Write(Level level, Channel channel, std::format_string<Args...> format,
           Args &&...args) noexcept {
  if (!Enabled(level, channel)) {
    return;
  }
  try {
    Write(level, channel, std::format(format, std::forward<Args>(args)...));
  } catch (...) {
    Write(Level::Error, Channel::Application, "Failed to format a log message");
  }
}

template <typename... Args>
void Trace(Channel channel, std::format_string<Args...> format,
           Args &&...args) noexcept {
  Write(Level::Trace, channel, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Debug(Channel channel, std::format_string<Args...> format,
           Args &&...args) noexcept {
  Write(Level::Debug, channel, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Info(Channel channel, std::format_string<Args...> format,
          Args &&...args) noexcept {
  Write(Level::Info, channel, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Warning(Channel channel, std::format_string<Args...> format,
             Args &&...args) noexcept {
  Write(Level::Warning, channel, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Error(Channel channel, std::format_string<Args...> format,
           Args &&...args) noexcept {
  Write(Level::Error, channel, format, std::forward<Args>(args)...);
}

template <typename... Args>
void Critical(Channel channel, std::format_string<Args...> format,
              Args &&...args) noexcept {
  Write(Level::Critical, channel, format, std::forward<Args>(args)...);
}

} // namespace logging
