///
/// PCM sound interface
///
#pragma once

#include "volume.h"

#include "audio/constants.h"
#include "sys/buffer.h"
#include "util/enum_flags.h"

// Constants & macros
using SND_INSTANCE_ID = uint8_t;

#define SND_OBJ_MAX 30 // Maximum number of SE types

extern const uint8_t &Snd_BGMTempoNum;
extern const uint8_t &Snd_BGMTempoDenom;

void Snd_Cleanup(void);
void Snd_SetVolumes(VOLUME bgm, VOLUME se);
[[nodiscard]] VOLUME Snd_BGMVolume(void);
[[nodiscard]] VOLUME Snd_SEVolume(void);
void Snd_UpdateVolumes(void);

// BGM
// ---

extern float Snd_BGMGainFactor;

bool Snd_BGMInit(void);
void Snd_BGMCleanup(void);
// ---

bool Snd_SEInit(void);
void Snd_SECleanup(void);

bool Snd_SELoad(BYTE_BUFFER_OWNED buffer, uint8_t id, SND_INSTANCE_ID max);

// Playback & stop
void Snd_SEPlay(SfxId id, int x = SND_X_MID, bool loop = false);
void Snd_SEStop(SfxId id);
void Snd_SEStopAll(void);
