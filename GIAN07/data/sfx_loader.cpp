///
/// SfxLoader - installs the game's sound-effect bank into the audio backend
///

#include <array>
#include <cstddef>
#include <cstdint>

#include "sfx_loader.h"

#include "audio/audio_system.h"

namespace data {

bool SfxLoader::Load() const {
  constexpr std::array<uint8_t, 20> kMaxInstances = {
      5, 5, 1, 1, 1, 1, 1, 1, 1, 1, 2, 5, 1, 1, 1, 1, 5, 1, 5, 1,
  };

  if ((audio_ == nullptr) || !audio_->IsEnabled()) {
    return false;
  }
  for (std::size_t id = 0; id < kMaxInstances.size(); ++id) {
    const auto buffer = data_->ExtractSound(id);
    if (!audio_->LoadSfx(static_cast<uint8_t>(id), buffer, kMaxInstances[id])
             .success) {
      return false;
    }
  }
  return true;
}

} // namespace data
