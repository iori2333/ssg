///
/// Platform-specific PCM sound interface
///

#pragma once

#include <chrono>
#include <span>

#include "snd.h"
struct SDL_AudioSpec;

bool AudioBackendInitialize();
void AudioBackendCleanup();

// The platform-independent layer always calls this after
// AudioBackendInitialize().
bool AudioBackendInitializeBgm();

// The platform-independent layer always calls this before
// AudioBackendCleanup().
void AudioBackendCleanupBgm();

namespace bgm {
struct Track;
}
bool AudioBackendLoadBgm(std::shared_ptr<bgm::Track> track);
void AudioBackendPlayBgm();
void AudioBackendStopBgm();

// Returns the amount of milliseconds that the subsystem has been playing the
// BGM track for.
std::chrono::milliseconds AudioBackendBgmPlayTime();

void AudioBackendUpdateBgmVolume();
void AudioBackendUpdateBgmTempo();

// The platform-independent layer always calls this after
// AudioBackendInitialize().
bool AudioBackendInitializeSoundEffects();

// The platform-independent layer always calls this before
// AudioBackendCleanup().
void AudioBackendCleanupSoundEffects();

void AudioBackendUpdateSoundEffectVolume();

bool AudioBackendLoadSoundEffect(uint8_t id, SoundInstanceId max,
                                 const SDL_AudioSpec &spec,
                                 std::span<const uint8_t> pcm);
void AudioBackendPlaySoundEffect(uint8_t id, float pan = 0.0f,
                                 bool loop = false);
void AudioBackendStopSoundEffect(uint8_t id);

// Pause or resume all playing sounds if the window loses focus
void AudioBackendPauseAll();
void AudioBackendResumeAll();
