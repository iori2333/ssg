#include "audio_types.h"
#include <string_view>

namespace audio {

std::string_view ToString(PlaybackState state) {
  switch (state) {
  case PlaybackState::Idle:
    return "idle";
  case PlaybackState::Loading:
    return "loading";
  case PlaybackState::Ready:
    return "ready";
  case PlaybackState::Starting:
    return "starting";
  case PlaybackState::Playing:
    return "playing";
  case PlaybackState::Paused:
    return "paused";
  case PlaybackState::Stopping:
    return "stopping";
  case PlaybackState::Faulted:
    return "faulted";
  }
  return "unknown";
}

std::string_view ToString(BgmMode mode) {
  switch (mode) {
  case BgmMode::None:
    return "none";
  case BgmMode::Waveform:
    return "waveform";
  case BgmMode::Midi:
    return "midi";
  }
  return "unknown";
}

} // namespace audio
