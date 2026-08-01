///
/// MIDI management functions
///
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

#include "volume.h"

#include "util/enum_flags.h"

enum class MidiFlags : uint8_t {
  None = 0x00,
  FixSysExBugs = 0x01,

  Mask = FixSysExBugs,
};

template <> inline constexpr bool util::EnableEnumFlags<MidiFlags> = true;

enum class MidiBackendState : uint8_t {
  Stopped,
  Playing,
  Paused,
};

// A position within a MIDI sequence. We need negative numbers for proper
// distance calculations.
using MidiPulse = int64_t;

// Sufficiently precise realtime values for MIDI event timing. Signed 64-bit
// integers can fit the highest possible tempo × delta time value at nanosecond
// precision (0xFF'FF'FF × 0xF'FF'FF'FF × 10³), with one bit to spare.
// (Nanoseconds only add a factor of 10³ here because tempo values already use
// microseconds.)
using MidiRealtime = std::chrono::nanoseconds;

struct MidiLoop {
  MidiPulse start = 0;
  MidiPulse end = 0;

  explicit operator bool() const { return (start != end); }
};

struct MidiPlayTime {
  MidiPulse pulse_of_last_event_processed = 0;
  MidiPulse pulse_interpolated = 0;

  // Total playback realtime of the current sequence.
  std::chrono::duration<int32_t, std::milli> realtime =
      (std::chrono::duration<int32_t, std::milli>::zero());
  MidiRealtime realtime_since_last_event = MidiRealtime::zero();
};

// Functions

// Returns the new current MIDI flags.
[[nodiscard]] MidiFlags MidiSetFlags(MidiFlags flags_new);
void MidiSetVolume(AudioVolume volume);
void MidiSetTempo(uint8_t numerator, uint8_t denominator);

// Starts outputting the loaded MIDI to the backend.
void MidiPlay(void); // Starts playback

// Stops backend output and resets the tables.
void MidiStop(void); // Stops playback

void MidiPause(void);
void MidiResume(void);

// Returns the current (not maximum) MIDI master volume.
AudioVolume MidiFadeVolume(void);

void MidiUpdateVolume(void);
void MidiFadeOut(AudioVolume volume_start, std::chrono::milliseconds duration);

bool MidiLoad(std::vector<uint8_t> buffer); // Load a MIDI file from a buffer

// Sets a loop point for the currently loaded sequence.
void MidiSetLoop(const MidiLoop &loop);

bool MidiIsLoaded(void);

struct MidiVisualization {
  std::array<std::array<uint8_t, 128>, 16> play{};
  std::array<std::array<uint8_t, 128>, 16> levels{};
  std::array<std::array<uint8_t, 128>, 16> notes{};
  std::array<std::array<uint8_t, 128>, 16> note_highlights{};
  std::array<uint8_t, 16> pan{};
  std::array<uint8_t, 16> expression{};
  std::array<uint8_t, 16> volume{};
  MidiPlayTime play_time{};
  bool loaded = false;
};

[[nodiscard]] MidiPlayTime MidiPlayTimePosition(void);
[[nodiscard]] MidiVisualization MidiVisualizationState(void);

std::string_view MidiTitle(void); // Returns the title of the current song

// Processes and outputs the next time [delta] of the currently loaded MIDI
// sequence.
void MidiProcess(MidiRealtime delta);

void MidiResetTables(void); // Initializes various tables
