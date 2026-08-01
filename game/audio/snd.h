///
/// PCM sound interface
///
#pragma once

#include <cstdint>
#include <span>

#include "volume.h"

// Constants & macros
using SND_INSTANCE_ID = uint8_t;

inline constexpr uint8_t SND_OBJ_MAX = 30;

void Snd_Cleanup(void);
void Snd_SetVolumes(VOLUME bgm, VOLUME se);
[[nodiscard]] VOLUME Snd_BGMVolume(void);
[[nodiscard]] VOLUME Snd_SEVolume(void);
void Snd_UpdateVolumes(void);

// BGM
// ---

bool Snd_BGMInit(void);
void Snd_BGMCleanup(void);
// ---

bool Snd_SEInit(void);
void Snd_SECleanup(void);

bool Snd_SELoad(std::span<const uint8_t> buffer, uint8_t id,
                SND_INSTANCE_ID max);

// Playback & stop
void Snd_SEPlay(uint8_t id, float pan = 0.0f, bool loop = false);
void Snd_SEStop(uint8_t id);
void Snd_SEStopAll(void);
