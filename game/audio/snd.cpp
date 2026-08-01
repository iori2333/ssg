///
/// Sound interface
///

#include <algorithm>
#include <cassert>

#include <SDL3/SDL_audio.h>

#include "snd.h"
#include "snd_backend.h"

#include "util/guard.h"

namespace {

struct SoundState {
  AudioVolume bgm_volume = kMaxAudioVolume;
  AudioVolume se_volume = kMaxAudioVolume;
  bool system_initialized = false;
  bool bgm_initialized = false;
  bool se_initialized = false;
};

SoundState &State() {
  static SoundState state;
  return state;
}

bool InitializeSystem() {
  auto &state = State();
  if (state.system_initialized) {
    return true;
  }
  assert(!state.bgm_initialized && !state.se_initialized);
  if (!AudioBackendInitialize()) {
    return false;
  }
  state.system_initialized = true;
  return true;
}

bool InitializeSubsystem(bool &initialized, bool (*initialize)()) {
  if (initialized) {
    return true;
  }
  if (!InitializeSystem() || !initialize()) {
    return false;
  }
  initialized = true;
  AudioUpdateVolumes();
  return true;
}

void CleanupSystemIfUnused() {
  auto &state = State();
  if (state.system_initialized && !state.bgm_initialized &&
      !state.se_initialized) {
    AudioBackendCleanup();
    state.system_initialized = false;
  }
}

void CleanupSubsystem(bool &initialized, void (*cleanup)()) {
  if (initialized) {
    cleanup();
    initialized = false;
  }
  CleanupSystemIfUnused();
}

} // namespace

void AudioCleanup() {
  auto &state = State();
  CleanupSubsystem(state.se_initialized, AudioBackendCleanupSoundEffects);
  CleanupSubsystem(state.bgm_initialized, AudioBackendCleanupBgm);
}

void AudioSetVolumes(AudioVolume bgm, AudioVolume se) {
  State().bgm_volume = std::min(bgm, kMaxAudioVolume);
  State().se_volume = std::min(se, kMaxAudioVolume);
  AudioUpdateVolumes();
}

AudioVolume AudioBgmVolume() { return State().bgm_volume; }

AudioVolume AudioSoundEffectVolume() { return State().se_volume; }

void AudioUpdateVolumes() {
  if (State().bgm_initialized) {
    AudioBackendUpdateBgmVolume();
  }
  if (State().se_initialized) {
    AudioBackendUpdateSoundEffectVolume();
  }
}

bool AudioInitializeBgm() {
  return InitializeSubsystem(State().bgm_initialized,
                             AudioBackendInitializeBgm);
}

bool AudioInitializeSoundEffects() {
  return InitializeSubsystem(State().se_initialized,
                             AudioBackendInitializeSoundEffects);
}

void AudioCleanupBgm() {
  CleanupSubsystem(State().bgm_initialized, AudioBackendCleanupBgm);
}

void AudioCleanupSoundEffects() {
  CleanupSubsystem(State().se_initialized, AudioBackendCleanupSoundEffects);
}

bool AudioLoadSoundEffect(std::span<const uint8_t> buffer, uint8_t id,
                          SoundInstanceId max) {
  auto *io = SDL_IOFromConstMem(buffer.data(), buffer.size());
  if (!io) {
    return false;
  }

  SDL_AudioSpec spec{};
  uint8_t *pcm_buf = nullptr;
  uint32_t pcm_len = 0;
  if (!SDL_LoadWAV_IO(io, true, &spec, &pcm_buf, &pcm_len)) {
    return false;
  }
  auto pcm_guard = util::MakeGuard(pcm_buf, SDL_free);
  return AudioBackendLoadSoundEffect(id, max, spec, {pcm_buf, pcm_len});
}

void AudioPlaySoundEffect(uint8_t id, float pan, bool loop) {
  return AudioBackendPlaySoundEffect(id, pan, loop);
}

void AudioStopSoundEffect(uint8_t id) {
  return AudioBackendStopSoundEffect(id);
}

void AudioStopAllSoundEffects() {
  for (auto i = 0; i < kSoundObjectCount; i++) {
    AudioBackendStopSoundEffect(i);
  }
}
