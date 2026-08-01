///
/// Platform-specific PCM sound interface
///

#pragma once

#include <chrono>
#include <span>

#include "snd.h"
struct SDL_AudioSpec;

bool AudioBackendInitialize(void);
void AudioBackendCleanup(void);

// The platform-independent layer always calls this after
// AudioBackendInitialize().
bool AudioBackendInitializeBgm(void);

// The platform-independent layer always calls this before
// AudioBackendCleanup().
void AudioBackendCleanupBgm(void);

namespace bgm {
struct Track;
}
bool AudioBackendLoadBgm(std::shared_ptr<bgm::Track> track);
void AudioBackendPlayBgm(void);
void AudioBackendStopBgm(void);

// Returns the amount of milliseconds that the subsystem has been playing the
// BGM track for.
std::chrono::milliseconds AudioBackendBgmPlayTime(void);

void AudioBackendUpdateBgmVolume(void);
void AudioBackendUpdateBgmTempo(void);

// The platform-independent layer always calls this after
// AudioBackendInitialize().
bool AudioBackendInitializeSoundEffects(void);

// The platform-independent layer always calls this before
// AudioBackendCleanup().
void AudioBackendCleanupSoundEffects(void);

void AudioBackendUpdateSoundEffectVolume(void);

bool AudioBackendLoadSoundEffect(uint8_t id, SoundInstanceId max,
                                 const SDL_AudioSpec &spec,
                                 std::span<const uint8_t> pcm);
void AudioBackendPlaySoundEffect(uint8_t id, float pan = 0.0f,
                                 bool loop = false);
void AudioBackendStopSoundEffect(uint8_t id);

// Pause or resume all playing sounds if the window loses focus
void AudioBackendPauseAll();
void AudioBackendResumeAll();
