///
/// Format-independent background music interface
///
#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

bool BGM_Init(std::string_view preferred_soundfont = {});
void BGM_Cleanup(void);

// General queries
// ---------------

enum class BGM_PLAYING {
  NONE,
  WAVEFORM,
  MIDI,
};

bool BGM_Enabled(void);
bool BGM_HasGainFactor(void);
bool BGM_GainApply(void);
BGM_PLAYING BGM_Playing(void);
std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void);
// ---------------

bool BGM_ChangeMIDIDevice(int8_t direction);

// Playback
// --------

void BGM_Play(void);
void BGM_Stop(void);

void BGM_Pause(void);
void BGM_Resume(void);
// --------

// Audio source loading. At most one of LoadWaveform/LoadMIDI is active
// at a time; loading one replaces the previous.
// Returns false if the source could not be opened/decoded.
bool BGM_LoadWaveform(std::string_view path);
bool BGM_LoadMIDI(std::vector<uint8_t> buf);

// Cached waveform title (from Vorbis comment metadata).
// Empty if no waveform is loaded or if the waveform has no title tag.
std::string_view BGM_WaveformTitle();

// Track number tracking — read/written by the track management layer.
unsigned int BGM_LoadedNum();
void BGM_SetLoadedNum(unsigned int n);

// Clears the waveform source, falling back to pure MIDI playback.
void BGM_ClearWaveform();

// Processes all MIDI events of a playing waveform track's source MIDI that
// have occurred since the last call to this function.
void BGM_UpdateMIDITables(void);

// Volume control
// --------------

// Changes the gain apply flag, and applies the result to any currently playing
// waveform track.
void BGM_SetGainApply(bool apply);

void BGM_UpdateVolume(void);

// Fade out (higher number = faster)
void BGM_FadeOut(uint8_t speed);
// --------------

// Tempo control
// -------------

inline constexpr uint8_t kBgmTempoDenominator = 1 << 7;
inline constexpr int8_t kBgmTempoMin = -100;
inline constexpr int8_t kBgmTempoMax = 100;

int8_t BGM_GetTempo(void);
void BGM_SetTempo(int8_t tempo); // Change tempo
float BGM_TempoFactor();
float BGM_GainFactor();
// -------------
