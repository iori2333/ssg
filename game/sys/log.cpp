/// Application-wide structured logging.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <system_error>
#ifdef PBG_DEBUG
#include <thread>
#endif

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#include "log.h"

#include "util/time_api.h"

namespace logging {
namespace {

std::mutex LogMutex;
constexpr auto ChannelCount = static_cast<size_t>(Channel::Sdl) + 1;
std::array<Level, ChannelCount> ChannelThresholds{};
std::unique_ptr<std::ofstream> LogFile;
bool ConsoleEnabled = false;
bool Initialized = false;
SDL_LogOutputFunction PreviousSdlOutput;
void *PreviousSdlUserdata = nullptr;

constexpr std::string_view LevelName(Level level) {
  switch (level) {
  case Level::Trace:
    return "TRACE";
  case Level::Debug:
    return "DEBUG";
  case Level::Info:
    return "INFO";
  case Level::Warning:
    return "WARNING";
  case Level::Error:
    return "ERROR";
  case Level::Critical:
    return "CRITICAL";
  case Level::Off:
    return "OFF";
  }
  return "UNKNOWN";
}

constexpr std::string_view ChannelName(Channel channel) {
  switch (channel) {
  case Channel::Application:
    return "app";
  case Channel::Config:
    return "config";
  case Channel::Data:
    return "data";
  case Channel::I18n:
    return "i18n";
  case Channel::GameFlow:
    return "gameflow";
  case Channel::Stage:
    return "stage";
  case Channel::Gameplay:
    return "gameplay";
  case Channel::Record:
    return "record";
  case Channel::Audio:
    return "audio";
  case Channel::Music:
    return "music";
  case Channel::Graphics:
    return "graphics";
  case Channel::Input:
    return "input";
  case Channel::Ui:
    return "ui";
  case Channel::Platform:
    return "platform";
  case Channel::Crash:
    return "crash";
  case Channel::Sdl:
    return "sdl";
  }
  return "unknown";
}

std::optional<Level> ParseLevel(std::string_view value) {
  if (value == "trace") {
    return Level::Trace;
  }
  if (value == "debug") {
    return Level::Debug;
  }
  if (value == "info") {
    return Level::Info;
  }
  if (value == "warning" || value == "warn") {
    return Level::Warning;
  }
  if (value == "error") {
    return Level::Error;
  }
  if (value == "critical") {
    return Level::Critical;
  }
  if (value == "off") {
    return Level::Off;
  }
  return std::nullopt;
}

std::optional<Channel> ParseChannel(std::string_view value) {
  for (size_t i = 0; i < ChannelCount; ++i) {
    const auto channel = static_cast<Channel>(i);
    if (ChannelName(channel) == value) {
      return channel;
    }
  }
  return std::nullopt;
}

void ApplyChannelOverrides(std::string_view config) {
  while (!config.empty()) {
    const auto comma = config.find(',');
    const auto entry = config.substr(0, comma);
    const auto separator = entry.find('=');
    if (separator != std::string_view::npos) {
      const auto channel_name = entry.substr(0, separator);
      const auto level = ParseLevel(entry.substr(separator + 1));
      if (level && channel_name == "*") {
        ChannelThresholds.fill(*level);
      } else if (level) {
        const auto channel = ParseChannel(channel_name);
        if (channel) {
          ChannelThresholds[static_cast<size_t>(*channel)] = *level;
        }
      }
    }
    if (comma == std::string_view::npos) {
      break;
    }
    config.remove_prefix(comma + 1);
  }
}

Level LevelFromSdl(SDL_LogPriority priority) {
  switch (priority) {
  case SDL_LOG_PRIORITY_TRACE:
  case SDL_LOG_PRIORITY_VERBOSE:
    return Level::Trace;
  case SDL_LOG_PRIORITY_DEBUG:
    return Level::Debug;
  case SDL_LOG_PRIORITY_INFO:
    return Level::Info;
  case SDL_LOG_PRIORITY_WARN:
    return Level::Warning;
  case SDL_LOG_PRIORITY_ERROR:
    return Level::Error;
  case SDL_LOG_PRIORITY_CRITICAL:
    return Level::Critical;
  case SDL_LOG_PRIORITY_INVALID:
  case SDL_LOG_PRIORITY_COUNT:
    return Level::Debug;
  }
  return Level::Debug;
}

Channel ChannelFromSdl(int category) {
  switch (category) {
  case SDL_LOG_CATEGORY_APPLICATION:
    return Channel::Application;
  case SDL_LOG_CATEGORY_AUDIO:
    return Channel::Audio;
  case SDL_LOG_CATEGORY_VIDEO:
  case SDL_LOG_CATEGORY_RENDER:
  case SDL_LOG_CATEGORY_GPU:
    return Channel::Graphics;
  case SDL_LOG_CATEGORY_INPUT:
    return Channel::Input;
  case SDL_LOG_CATEGORY_SYSTEM:
    return Channel::Platform;
  default:
    return Channel::Sdl;
  }
}

void WriteLine(std::string_view line, bool flush) {
  if (LogFile != nullptr && *LogFile) {
    *LogFile << line << '\n';
    if (flush) {
      LogFile->flush();
    }
  }
  if (ConsoleEnabled) {
    std::clog << line << '\n';
    if (flush) {
      std::clog.flush();
    }
  }
}

void SDLCALL SdlOutput(void * /*unused*/, int category,
                       SDL_LogPriority priority, const char *message) {
  Write(LevelFromSdl(priority), ChannelFromSdl(category), message);
}

} // namespace

bool Enabled(Level level, Channel channel) noexcept {
  return level >= ChannelThresholds[static_cast<size_t>(channel)] &&
         level != Level::Off;
}

void Write(Level level, Channel channel, std::string_view message) noexcept {
  if (!Enabled(level, channel)) {
    return;
  }

  try {
    const auto now = std::chrono::system_clock::now();
    const auto millis_since_epoch =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    const auto millis = ((millis_since_epoch % 1000) + 1000) % 1000;
    const auto local = util::LocalTime();
#ifdef PBG_DEBUG
    const auto prefix = std::format(
        "{:%Y-%m-%d %H:%M:%S}.{:03} [{}][{}][thread={}] ", local, millis,
        LevelName(level), ChannelName(channel), std::this_thread::get_id());
#else
    const auto prefix =
        std::format("{:%Y-%m-%d %H:%M:%S}.{:03} [{}][{}] ", local, millis,
                    LevelName(level), ChannelName(channel));
#endif
    const bool flush = level >= Level::Warning;

    std::scoped_lock const lock(LogMutex);
    size_t offset = 0;
    while (offset <= message.size()) {
      const auto end = message.find('\n', offset);
      const auto line = message.substr(
          offset, end == std::string_view::npos ? end : end - offset);
      WriteLine(std::format("{}{}", prefix, line), flush);
      if (end == std::string_view::npos) {
        break;
      }
      offset = end + 1;
    }
  } catch (...) {
    return;
  }
}

void Initialize(std::string_view base_directory, std::string_view application,
                std::string_view version) {
  if (Initialized) {
    return;
  }

#ifdef PBG_DEBUG
  constexpr auto default_level = Level::Debug;
  ConsoleEnabled = true;
#else
  constexpr auto default_level = Level::Info;
#endif

  auto threshold = default_level;
  const char *channel_config = nullptr;
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
  if (const char *value = std::getenv("SSG_LOG_LEVEL")) {
    threshold = ParseLevel(value).value_or(threshold);
  }
  channel_config = std::getenv("SSG_LOG");
#if defined(__clang__) && defined(_WIN32)
#pragma clang diagnostic pop
#endif
  ChannelThresholds.fill(threshold);
  ChannelThresholds[static_cast<size_t>(Channel::Sdl)] =
      std::max(threshold, Level::Info);
  if (channel_config != nullptr) {
    ApplyChannelOverrides(channel_config);
  }

  std::error_code error;
  auto root = std::filesystem::path(base_directory);
  if (root.empty()) {
    root = std::filesystem::current_path(error);
  }
  const auto directory = root / "logs";
  std::filesystem::create_directories(directory, error);
  if (!error) {
    const auto current = directory / "GIAN07.log";
    const auto previous = directory / "GIAN07.previous.log";
    std::filesystem::remove(previous, error);
    error.clear();
    if (std::filesystem::is_regular_file(current, error)) {
      error.clear();
      std::filesystem::rename(current, previous, error);
    }
    error.clear();
    LogFile = std::make_unique<std::ofstream>();
    LogFile->open(current, std::ios::binary | std::ios::trunc);
  }
  if (LogFile == nullptr || !*LogFile) {
    ConsoleEnabled = true;
  }

  SDL_GetLogOutputFunction(&PreviousSdlOutput, &PreviousSdlUserdata);
  SDL_SetLogOutputFunction(SdlOutput, nullptr);
  Initialized = true;
  Info(Channel::Application, "Starting {} {} (log level: {})", application,
       version, LevelName(threshold));
}

void Flush() noexcept {
  try {
    std::scoped_lock const lock(LogMutex);
    if (LogFile != nullptr && *LogFile) {
      LogFile->flush();
    }
    if (ConsoleEnabled) {
      std::clog.flush();
    }
  } catch (...) {
    return;
  }
}

void Shutdown() {
  if (!Initialized) {
    return;
  }
  Info(Channel::Application, "Logging stopped");
  SDL_SetLogOutputFunction(PreviousSdlOutput, PreviousSdlUserdata);
  Flush();
  {
    std::scoped_lock const lock(LogMutex);
    if (LogFile != nullptr) {
      LogFile->close();
      LogFile.reset();
    }
  }
  Initialized = false;
}

void SdlError(Channel channel, std::string_view context) noexcept {
  Error(channel, "{}: {}", context, SDL_GetError());
}

} // namespace logging
