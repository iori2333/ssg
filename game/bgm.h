///
/// Format-independent background music interface
///
#pragma once

#include "game/hash.h"
#include "platform/buffer.h"
#include <chrono>
#include <cstdint>
#include <functional>
#include <string_view>

// Loads the BGM with the given 0-based [id] from the game's original BGM data
// source.
extern bool (*const BGM_MidLoadOriginal)(unsigned int id);

// Loads MIDI BGM from the given byte buffer.
extern bool (*const BGM_MidLoadBuffer)(BYTE_BUFFER_OWNED);

// Loads the source MIDI via its hash from the game's original BGM data source.
extern bool (*const BGM_MidLoadByHash)(const HASH &hash);

bool BGM_Init(void);
void BGM_Cleanup(void);

// General queries
// ---------------

enum class BGM_PLAYING {
  NONE,
  WAVEFORM,
  MIDI,
};

bool BGM_Enabled(void);
bool BGM_LoadedOriginalMIDI(void);
bool BGM_HasGainFactor(void);
bool BGM_GainApply(void);
BGM_PLAYING BGM_Playing(void);
std::chrono::duration<int32_t, std::milli> BGM_PlayTime(void);
std::string_view BGM_Title(void);
// ---------------

bool BGM_ChangeMIDIDevice(int8_t direction); // Change output device

// Playback
// --------

// Stops the currently playing BGM, then loads and plays the track with the
// given 0-based [id]. Returns `true` if the BGM was changed successfully.
bool BGM_Switch(unsigned int id);

void BGM_Play(void);
void BGM_Stop(void);

void BGM_Pause(void);
void BGM_Resume(void);
// --------

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

static constexpr uint8_t BGM_TEMPO_DENOM = (1 << 7); // Default tempo
static constexpr int8_t BGM_TEMPO_MIN = -100;
static constexpr int8_t BGM_TEMPO_MAX = 100;

int8_t BGM_GetTempo(void);
void BGM_SetTempo(int8_t tempo); // Change tempo
// -------------

// BGM pack management
// -------------------

// Returns whether at least one BGM pack exists under the BGM root directory.
// The result is cached and invalidated whenever BGM is paused.
bool BGM_PacksAvailable(bool invalidate_cache = false);

size_t BGM_PackCount(void);
void BGM_PackForeach(std::function<void(std::string_view pack)> func);

// Restarts any currently playing BGM when switching to a different [pack].
// Returns `false` if the given [pack] doesn't exist, and switches to the empty
// pack in that case.
bool BGM_PackSet(std::string_view pack);
// -------------------
