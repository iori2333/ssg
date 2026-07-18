///
/// Platform-specific MIDI backend interface
///

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

// Describes the source of a SoundFont file for display purposes.
enum class MID_BACKEND_DEVICE_SOURCE : uint8_t {
  LOCAL,  // Project-local soundfonts/ directory
  SYSTEM, // System path (gm.dls, /usr/share/soundfonts, etc.)
  ENV,    // DEFAULT_SOUNDFONT environment variable
};

// Initializes the backend with a preferred SoundFont. If [preferred_soundfont]
// is empty,  the first found font is used.
bool MidBackend_Init(std::string_view preferred_soundfont = {});

// Returns the basename of the currently active SoundFont, or std::nullopt
// if the backend is not initialized.
std::optional<std::string_view> MidBackend_CurrentSoundFont();

// Shuts down the backend.
void MidBackend_Cleanup(void); // MIDI-related cleanup

// Returns the name of the current MIDI device, or std::nullopt if the backend
// is not initialized. Can also be used for general initialization checks.
std::optional<std::string_view> MidBackend_DeviceName(void);

// Returns the number of available devices (SoundFont files).
size_t MidBackend_DeviceCount(void);

// Returns the name of the device at the given zero-based [index], or
// std::nullopt if [index] is out of range or the backend is not initialized.
std::optional<std::string_view> MidBackend_DeviceNameAt(size_t index);

// Returns the source of the device at the given zero-based [index], or
// std::nullopt if [index] is out of range or the backend is not initialized.
std::optional<MID_BACKEND_DEVICE_SOURCE>
MidBackend_DeviceSource(size_t index);

// Switches to the next working output device in the given positive or negative
// [direction].
bool MidBackend_DeviceChange(int8_t direction); // Change output device

// Switches to the device at the given zero-based [index]. Returns false if
// [index] is out of range.
bool MidBackend_DeviceSelect(size_t index); // Select device by index

// Starts a timer that periodically calls Mid_Proc().
void MidBackend_StartTimer(void);

// Stops the timer.
void MidBackend_StopTimer(void);

// Sends a raw MIDI event to the active device. [event] unfortunately has to be
// mutable because MIDIHDR on Windows wants a non-const pointer...
void MidBackend_Out(uint8_t byte_1, uint8_t byte_2, uint8_t byte_3 = 0x00);
void MidBackend_Out(std::span<uint8_t> event);

// Stops all currently playing notes.
void MidBackend_Panic(void);
