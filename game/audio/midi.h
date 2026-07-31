///
/// MIDI management functions
///
#ifndef PBGWIN_PBGMIDI_H
#define PBGWIN_PBGMIDI_H "PBGMIDI : Version 0.31 : Update 2000/08/04"

// Revision history
// 2000/08/04 : Made Mid_Free() global.

// 2000/03/22 : Changed fade-out function from master volume to CC volume
//            : If there are MIDI backends that cannot keep up,
//            : the message sending may need to be improved.

#include <array>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

#include "volume.h"

enum class MID_FLAGS : uint8_t {
  HAS_BITFLAG_OPERATORS,
  NONE = 0x00,
  FIX_SYSEX_BUGS = 0x01,

  MASK = FIX_SYSEX_BUGS,
};

enum class MID_BACKEND_STATE : uint8_t {
  STOP,  // Stopped
  PLAY,  // Playing
  PAUSE, // Paused
};

// A position within a MIDI sequence. We need negative numbers for proper
// distance calculations.
using MID_PULSE = int64_t;

// Sufficiently precise realtime values for MIDI event timing. Signed 64-bit
// integers can fit the highest possible tempo × delta time value at nanosecond
// precision (0xFF'FF'FF × 0xF'FF'FF'FF × 10³), with one bit to spare.
// (Nanoseconds only add a factor of 10³ here because tempo values already use
// microseconds.)
using MID_REALTIME = std::chrono::nanoseconds;

struct MID_LOOP {
  MID_PULSE start = 0;
  MID_PULSE end = 0;

  explicit operator bool() const { return (start != end); }
};

struct MID_PLAYTIME {
  MID_PULSE pulse_of_last_event_processed = 0;
  MID_PULSE pulse_interpolated = 0;

  // Total playback realtime of the current sequence.
  std::chrono::duration<int32_t, std::milli> realtime =
      (std::chrono::duration<int32_t, std::milli>::zero());
  MID_REALTIME realtime_since_last_event = MID_REALTIME::zero();
};

// Functions

// Returns the new current MIDI flags.
[[nodiscard]] MID_FLAGS Mid_SetFlags(MID_FLAGS flags_new);
void Mid_SetVolume(VOLUME volume);
void Mid_SetTempo(uint8_t numerator, uint8_t denominator);

// Starts outputting the loaded MIDI to the backend.
void Mid_Play(void); // Starts playback

// Stops backend output and resets the tables.
void Mid_Stop(void); // Stops playback

void Mid_Pause(void);
void Mid_Resume(void);

// Returns the current (not maximum) MIDI master volume.
VOLUME Mid_GetFadeVolume(void);

void Mid_UpdateVolume(void);
void Mid_FadeOut(VOLUME volume_start, std::chrono::milliseconds duration);

bool Mid_Load(std::vector<uint8_t> buffer); // Load a MIDI file from a buffer

// Sets a loop point for the currently loaded sequence.
void Mid_SetLoop(const MID_LOOP &loop);

bool Mid_Loaded(void);

struct MID_VISUALIZATION {
  std::array<std::array<uint8_t, 128>, 16> play{};
  std::array<std::array<uint8_t, 128>, 16> levels{};
  std::array<std::array<uint8_t, 128>, 16> notes{};
  std::array<std::array<uint8_t, 128>, 16> note_highlights{};
  std::array<uint8_t, 16> pan{};
  std::array<uint8_t, 16> expression{};
  std::array<uint8_t, 16> volume{};
  MID_PLAYTIME play_time{};
  bool loaded = false;
};

[[nodiscard]] MID_PLAYTIME Mid_GetPlayTime(void);
[[nodiscard]] MID_VISUALIZATION Mid_GetVisualization(void);

std::string_view Mid_GetTitle(void); // Returns the title of the current song

// Processes and outputs the next time [delta] of the currently loaded MIDI
// sequence.
void Mid_Proc(MID_REALTIME delta);

void Mid_TableInit(void); // Initializes various tables

#endif
