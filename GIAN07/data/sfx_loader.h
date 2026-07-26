///
/// SfxLoader - installs the game's sound-effect bank into the audio backend
///
#pragma once

#include "game_data.h"

namespace data {

class SfxLoader {
public:
  explicit SfxLoader(const GameData &data) : data_(&data) {}

  [[nodiscard]] bool Load() const;

private:
  const GameData *data_;
};

} // namespace data
