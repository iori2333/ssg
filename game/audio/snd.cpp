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
  VOLUME bgm_volume = VOLUME_MAX;
  VOLUME se_volume = VOLUME_MAX;
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
  if (!SndBackend_Init()) {
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
  Snd_UpdateVolumes();
  return true;
}

void CleanupSystemIfUnused() {
  auto &state = State();
  if (state.system_initialized && !state.bgm_initialized &&
      !state.se_initialized) {
    SndBackend_Cleanup();
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

void Snd_Cleanup(void) {
  auto &state = State();
  CleanupSubsystem(state.se_initialized, SndBackend_SECleanup);
  CleanupSubsystem(state.bgm_initialized, SndBackend_BGMCleanup);
}

void Snd_SetVolumes(VOLUME bgm, VOLUME se) {
  State().bgm_volume = std::min(bgm, VOLUME_MAX);
  State().se_volume = std::min(se, VOLUME_MAX);
  Snd_UpdateVolumes();
}

VOLUME Snd_BGMVolume(void) { return State().bgm_volume; }

VOLUME Snd_SEVolume(void) { return State().se_volume; }

void Snd_UpdateVolumes(void) {
  if (State().bgm_initialized) {
    SndBackend_BGMUpdateVolume();
  }
  if (State().se_initialized) {
    SndBackend_SEUpdateVolume();
  }
}

bool Snd_BGMInit(void) {
  return InitializeSubsystem(State().bgm_initialized, SndBackend_BGMInit);
}

bool Snd_SEInit(void) {
  return InitializeSubsystem(State().se_initialized, SndBackend_SEInit);
}

void Snd_BGMCleanup(void) {
  CleanupSubsystem(State().bgm_initialized, SndBackend_BGMCleanup);
}

void Snd_SECleanup(void) {
  CleanupSubsystem(State().se_initialized, SndBackend_SECleanup);
}

bool Snd_SELoad(std::span<const uint8_t> buffer, uint8_t id,
                SND_INSTANCE_ID max) {
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
  return SndBackend_SELoad(id, max, spec, {pcm_buf, pcm_len});
}

void Snd_SEPlay(uint8_t id, float pan, bool loop) {
  return SndBackend_SEPlay(id, pan, loop);
}

void Snd_SEStop(uint8_t id) { return SndBackend_SEStop(id); }

void Snd_SEStopAll(void) {
  for (auto i = 0; i < SND_OBJ_MAX; i++) {
    SndBackend_SEStop(i);
  }
}
