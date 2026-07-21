///
/// SfxManager — sound effect loading
///
#include "sfx_manager.h"

#include "audio/snd.h"
#include "core/config.h"
#include "core/lz_uty.h"
#include "gameflow/gameflow_manager.h"
#include "pack_manager.h"

namespace {

constexpr uint8_t kSfxMax[] = {
    5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 5, 1, 1, 1, 1, 5, 1, 5, 1,
};

bool SndLoadFromPack(const PackFile &in, uint32_t filno, uint8_t id,
                     int max) {
  return Snd_SELoad(in.Extract(filno), id, max);
}

} // namespace

bool SfxManager::LoadAllFromPack(const PackFile &in) {
  if (!GameFlow.ctx.cfg->audio.se_enabled || !Snd_SEInit()) {
    GameFlow.ctx.cfg->audio.se_enabled = false;
    return false;
  }

  for (uint8_t id = 0; id < std::size(kSfxMax); id++) {
    if (!SndLoadFromPack(in, id, id, kSfxMax[id])) {
      GameFlow.ctx.cfg->audio.se_enabled = false;
      Snd_SECleanup();
      return false;
    }
  }

  Snd_UpdateVolumes();
  return true;
}

bool SfxManager::LoadAll() { return LoadAllFromPack(packs.Sound()); }
