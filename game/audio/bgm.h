///
/// Format-independent background music interface
///
#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

bool BgmInitialize(std::string_view preferred_soundfont = {});
void BgmCleanup();

// General queries
// ---------------

enum class BgmPlaybackSource {
  None,
  Waveform,
  Midi,
};

bool BgmIsEnabled();
bool BgmHasGainFactor();
bool BgmIsGainApplied();
BgmPlaybackSource BgmPlayingSource();
std::chrono::duration<int32_t, std::milli> BgmPlayTime();
// ---------------

bool BgmChangeMidiDevice(int8_t direction);

// Playback
// --------

void BgmPlay();
void BgmStop();

void BgmPause();
void BgmResume();
// --------

// Audio source loading. At most one of LoadWaveform/LoadMIDI is active
// at a time; loading one replaces the previous.
// Returns false if the source could not be opened/decoded.
bool BgmLoadWaveform(std::string_view path);
bool BgmLoadMidi(std::vector<uint8_t> buf);

// Cached waveform title (from Vorbis comment metadata).
// Empty if no waveform is loaded or if the waveform has no title tag.
std::string_view BgmWaveformTitle();

// Track number tracking — read/written by the track management layer.
unsigned int BgmLoadedTrackNumber();
void BgmSetLoadedTrackNumber(unsigned int n);

// Clears the waveform source, falling back to pure MIDI playback.
void BgmClearWaveform();

// Processes all MIDI events of a playing waveform track's source MIDI that
// have occurred since the last call to this function.
void BgmUpdateMidiTables();

// Volume control
// --------------

// Changes the gain apply flag, and applies the result to any currently playing
// waveform track.
void BgmSetGainApplied(bool apply);

void BgmUpdateVolume();

// Fade out (higher number = faster)
void BgmFadeOut(uint8_t speed);
// --------------

// Tempo control
// -------------

inline constexpr uint8_t kBgmTempoDenominator = 1 << 7;
inline constexpr int8_t kBgmTempoMin = -100;
inline constexpr int8_t kBgmTempoMax = 100;

int8_t BgmTempo();
void BgmSetTempo(int8_t tempo); // Change tempo
float BgmTempoFactor();
float BgmGainFactor();
// -------------
