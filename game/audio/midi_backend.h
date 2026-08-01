///
/// Platform-specific MIDI backend interface
///

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

// Describes the source of a SoundFont file for display purposes.
enum class MidiDeviceSource : uint8_t {
  Local,       // Project-local soundfonts/ directory
  System,      // System path (gm.dls, /usr/share/soundfonts, etc.)
  Environment, // DEFAULT_SOUNDFONT environment variable
};

// Initializes the backend with a preferred SoundFont. If [preferred_soundfont]
// is empty,  the first found font is used.
bool MidiBackendInitialize(std::string_view preferred_soundfont = {});

// Returns the basename of the currently active SoundFont, or std::nullopt
// if the backend is not initialized.
std::optional<std::string_view> MidiBackendCurrentSoundFont();

// Shuts down the backend.
void MidiBackendCleanup(); // MIDI-related cleanup

// Returns the name of the current MIDI device, or std::nullopt if the backend
// is not initialized. Can also be used for general initialization checks.
std::optional<std::string_view> MidiBackendDeviceName();

// Returns the number of available devices (SoundFont files).
size_t MidiBackendDeviceCount();

// Returns the name of the device at the given zero-based [index], or
// std::nullopt if [index] is out of range or the backend is not initialized.
std::optional<std::string_view> MidiBackendDeviceNameAt(size_t index);

// Returns the source of the device at the given zero-based [index], or
// std::nullopt if [index] is out of range or the backend is not initialized.
std::optional<MidiDeviceSource> MidiBackendDeviceSource(size_t index);

// Switches to the next working output device in the given positive or negative
// [direction].
bool MidiBackendChangeDevice(int8_t direction); // Change output device

// Switches to the device at the given zero-based [index]. Returns false if
// [index] is out of range.
bool MidiBackendSelectDevice(size_t index); // Select device by index

// Starts a timer that periodically calls MidiProcess().
void MidiBackendStartTimer();

// Stops the timer.
void MidiBackendStopTimer();

// Sends a raw MIDI event to the active device. [event] unfortunately has to be
// mutable because MIDIHDR on Windows wants a non-const pointer...
void MidiBackendOutput(uint8_t byte_1, uint8_t byte_2, uint8_t byte_3 = 0x00);
void MidiBackendOutput(std::span<uint8_t> event);

// Stops all currently playing notes.
void MidiBackendPanic();
