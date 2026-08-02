/// Shared audio types for the application-owned audio system.

#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

namespace audio {

enum class AudioError : uint8_t {
  None,
  NotInitialized,
  AlreadyInitialized,
  NotEnabled,
  InvalidArgument,
  TrackOpenFailed,
  MidiParseFailed,
  SoundFontLoadFailed,
  SoundFontRollbackFailed,
  DeviceSwitchFailed,
  DecodeFailed,
  BackendFailed,
};

struct AudioResult {
  bool success = false;
  AudioError error = AudioError::None;
  std::string message;

  static constexpr AudioResult Ok() { return {.success = true}; }

  static AudioResult Fail(AudioError error, std::string_view message) {
    return {.success = false, .error = error, .message = std::string{message}};
  }
};

enum class PlaybackState : uint8_t {
  Idle,
  Loading,
  Ready,
  Starting,
  Playing,
  Paused,
  Stopping,
  Faulted,
};

enum class BgmMode : uint8_t {
  None,
  Waveform,
  Midi,
};

struct BgmSnapshot {
  BgmMode mode = BgmMode::None;
  PlaybackState state = PlaybackState::Idle;
  std::string title;
  std::chrono::milliseconds play_time{};
  int tempo = 0;
};

using Volume = uint8_t;
inline constexpr Volume kMaxVolume = 127;

[[nodiscard]] std::string_view ToString(PlaybackState state);
[[nodiscard]] std::string_view ToString(BgmMode mode);

} // namespace audio
