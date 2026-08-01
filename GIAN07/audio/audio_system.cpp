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
  (void)MidiSetFlags(config.fix_sysex_bugs ? MidiFlags::FixSysExBugs
                                           : MidiFlags::None);

  const auto bgm_available =
      !config.bgm_enabled || BgmInitialize(config.soundfont);
  SetNormalization(config.bgm_vol_norm);
  initialized_ = true;
  return bgm_available;
}

void AudioSystem::Shutdown() {
  if (!initialized_) {
    return;
  }
  BgmCleanup();
  AudioCleanup();
  initialized_ = false;
}

bool AudioSystem::EnableBgm(bool enabled, std::string_view soundfont) {
  if (!enabled) {
    BgmCleanup();
    return true;
  }
  if (!BgmInitialize(soundfont)) {
    return false;
  }
  if (!music_.Play(0)) {
    BgmCleanup();
    return false;
  }
  return true;
}

bool AudioSystem::EnableSfx(bool enabled) {
  if (!enabled) {
    AudioCleanupSoundEffects();
    return true;
  }
  return sound_effects_.Load();
}

void AudioSystem::SetVolumes(AudioVolume bgm, AudioVolume sfx) {
  MidiSetVolume(bgm);
  AudioSetVolumes(bgm, sfx);
}

void AudioSystem::SetNormalization(bool enabled) { BgmSetGainApplied(enabled); }
