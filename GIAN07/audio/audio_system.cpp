/// Application-level audio lifecycle and configuration.

#include "audio_system.h"

#include "audio/bgm.h"
#include "audio/midi.h"
#include "audio/snd.h"
#include "data/sfx_loader.h"
#include "music/music_player.h"
#include "settings/config.h"

AudioSystem::~AudioSystem() { Shutdown(); }

bool AudioSystem::Initialize(const AudioConfig &config) {
  SetVolumes(config.bgm_volume, config.se_volume);
  (void)Mid_SetFlags(config.fix_sysex_bugs ? MID_FLAGS::FIX_SYSEX_BUGS
                                          : MID_FLAGS::NONE);

  const auto bgm_available =
      !config.bgm_enabled || BGM_Init(config.soundfont);
  SetNormalization(config.bgm_vol_norm);
  initialized_ = true;
  return bgm_available;
}

void AudioSystem::Shutdown() {
  if (!initialized_) {
    return;
  }
  BGM_Cleanup();
  Snd_Cleanup();
  initialized_ = false;
}

bool AudioSystem::EnableBgm(bool enabled, std::string_view soundfont) {
  if (!enabled) {
    BGM_Cleanup();
    return true;
  }
  if (!BGM_Init(soundfont)) {
    return false;
  }
  if (!music_.Play(0)) {
    BGM_Cleanup();
    return false;
  }
  return true;
}

bool AudioSystem::EnableSfx(bool enabled) {
  if (!enabled) {
    Snd_SECleanup();
    return true;
  }
  return sound_effects_.Load();
}

void AudioSystem::SetVolumes(VOLUME bgm, VOLUME sfx) {
  Mid_SetVolume(bgm);
  Snd_SetVolumes(bgm, sfx);
}

void AudioSystem::SetNormalization(bool enabled) {
  BGM_SetGainApply(enabled);
}
