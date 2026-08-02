/// Playback state definitions shared by audio components.

#pragma once

#include <cstdint>

namespace audio {

enum class SfxPlaybackState : uint8_t {
  Stopped,
  Playing,
  Paused,
};

enum class MidiSynthState : uint8_t {
  Uninitialized,
  Ready,
  Starting,
  Playing,
  Paused,
  Faulted,
};

} // namespace audio
