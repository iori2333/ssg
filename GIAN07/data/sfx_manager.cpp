///
/// SfxManager — sound effect loading
///
#include "sfx_manager.h"

#include "audio/snd.h"
#include "core/config.h"
#include "core/lz_uty.h"
#include "pack_manager.h"

namespace {

constexpr uint8_t kSfxMax[] = {
    5, 5, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 5, 1, 1, 1, 1, 5, 1, 5, 1,
};

bool SndLoadFromPack(const PACKFILE_READ &in, fil_no_t filno, uint8_t id,
                     int max) {
  return Snd_SELoad(in.MemExpand(filno), id, max);
}

} // namespace

bool SfxManager::LoadAllFromPack(const PACKFILE_READ &in) {
  if (!ConfigDat.se_enabled || !Snd_SEInit()) {
    ConfigDat.se_enabled = false;
    return false;
  }

  for (uint8_t id = 0; id < std::size(kSfxMax); id++) {
    if (!SndLoadFromPack(in, id, id, kSfxMax[id])) {
      ConfigDat.se_enabled = false;
      Snd_SECleanup();
      return false;
    }
  }

  Snd_UpdateVolumes();
  return true;
}

bool SfxManager::LoadAll() { return LoadAllFromPack(packs.Sound()); }
