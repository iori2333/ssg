///
/// SfxLoader - installs the game's sound-effect bank into the audio backend
///
#include <array>
#include <cstdint>

#include "sfx_loader.h"

#include "audio/snd.h"

namespace data {

bool SfxLoader::Load() const {
  constexpr std::array<uint8_t, 20> kMaxInstances = {
      5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 2, 5, 1, 1, 1, 1, 5, 1, 5, 1,
  };

  if (!AudioInitializeSoundEffects()) {
    return false;
  }
  for (uint8_t id = 0; id < kMaxInstances.size(); ++id) {
    if (!AudioLoadSoundEffect(data_->ExtractSound(id), id, kMaxInstances[id])) {
      AudioCleanupSoundEffects();
      return false;
    }
  }
  AudioUpdateVolumes();
  return true;
}

} // namespace data
