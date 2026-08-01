///
/// PCM sound interface
///
#pragma once

#include <cstdint>
#include <span>

#include "volume.h"

// Constants & macros
using SoundInstanceId = uint8_t;

inline constexpr uint8_t kSoundObjectCount = 30;

void AudioCleanup(void);
void AudioSetVolumes(AudioVolume bgm, AudioVolume se);
[[nodiscard]] AudioVolume AudioBgmVolume(void);
[[nodiscard]] AudioVolume AudioSoundEffectVolume(void);
void AudioUpdateVolumes(void);

// BGM
// ---

bool AudioInitializeBgm(void);
void AudioCleanupBgm(void);
// ---

bool AudioInitializeSoundEffects(void);
void AudioCleanupSoundEffects(void);

bool AudioLoadSoundEffect(std::span<const uint8_t> buffer, uint8_t id,
                          SoundInstanceId max);

// Playback & stop
void AudioPlaySoundEffect(uint8_t id, float pan = 0.0f, bool loop = false);
void AudioStopSoundEffect(uint8_t id);
void AudioStopAllSoundEffects(void);
