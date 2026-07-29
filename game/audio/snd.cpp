///
/// Sound interface
///

#include <algorithm>
#include <cassert>

#include <SDL3/SDL_audio.h>

#include "snd.h"
#include "snd_backend.h"

#include "util/enum_flags.h"
#include "util/guard.h"

float Snd_BGMGainFactor = 1.0f;
static VOLUME BGMVolume = VOLUME_MAX;
static VOLUME SEVolume = VOLUME_MAX;

static enum class SND_SYS {
  HAS_BITFLAG_OPERATORS,
  NOTHING = 0x0,
  SYSTEM = 0x1,
  BGM = 0x2,
  SE = 0x4,
} Initialized;

bool Snd_SystemInit(void) {
  if (!!(Initialized & SND_SYS::SYSTEM)) {
    return true;
  }
  assert(Initialized == SND_SYS::NOTHING);
  if (!SndBackend_Init()) {
    return false;
  }
  Initialized |= SND_SYS::SYSTEM;
  return true;
}

bool Snd_SubsystemInit(SND_SYS sys, bool (&SubsystemInit)(void)) {
  if (!!(Initialized & sys)) {
    return true;
  } else if (!Snd_SystemInit() || !SubsystemInit()) {
    return false;
  }
  Initialized |= sys;
  Snd_UpdateVolumes();
  return true;
}

void Snd_Cleanup(SND_SYS sys) {
  auto cleanup_sys = [](SND_SYS should, SND_SYS sys, void (&cleanup)(void)) {
    if (!!(should & sys) && !!(Initialized & sys)) {
      cleanup();
      Initialized &= ~sys;
    }
  };

  cleanup_sys(sys, SND_SYS::SE, SndBackend_SECleanup);
  cleanup_sys(sys, SND_SYS::BGM, SndBackend_BGMCleanup);

  // Silly double negation to work around C26813...
  if (~Initialized == ~SND_SYS::SYSTEM) {
    SndBackend_Cleanup();
    Initialized = SND_SYS::NOTHING;
  }
}

void Snd_Cleanup(void) { Snd_Cleanup(SND_SYS::BGM | SND_SYS::SE); }

void Snd_SetVolumes(VOLUME bgm, VOLUME se) {
  BGMVolume = std::min(bgm, VOLUME_MAX);
  SEVolume = std::min(se, VOLUME_MAX);
  Snd_UpdateVolumes();
}

VOLUME Snd_BGMVolume(void) { return BGMVolume; }

VOLUME Snd_SEVolume(void) { return SEVolume; }

void Snd_UpdateVolumes(void) {
  if (!!(Initialized & SND_SYS::BGM)) {
    SndBackend_BGMUpdateVolume();
  }
  if (!!(Initialized & SND_SYS::SE)) {
    SndBackend_SEUpdateVolume();
  }
}

bool Snd_BGMInit(void) {
  return Snd_SubsystemInit(SND_SYS::BGM, SndBackend_BGMInit);
}

bool Snd_SEInit(void) {
  return Snd_SubsystemInit(SND_SYS::SE, SndBackend_SEInit);
}

void Snd_BGMCleanup(void) { Snd_Cleanup(SND_SYS::BGM); }

void Snd_SECleanup(void) { Snd_Cleanup(SND_SYS::SE); }

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
  auto pcm_guard = make_guard(pcm_buf, SDL_free);
  return SndBackend_SELoad(id, max, spec, {pcm_buf, pcm_len});
}

void Snd_SEPlay(SfxId id, int x, bool loop) {
  return SndBackend_SEPlay(id, x, loop);
}

void Snd_SEStop(SfxId id) { return SndBackend_SEStop(std::to_underlying(id)); }

void Snd_SEStopAll(void) {
  for (auto i = 0; i < SND_OBJ_MAX; i++) {
    SndBackend_SEStop(i);
  }
}
