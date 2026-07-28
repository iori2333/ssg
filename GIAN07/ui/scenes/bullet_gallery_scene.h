/// Debug bullet gallery scene.

#pragma once

#include <cstdint>

#include "sys/input.h"

class BulletManager;
class Player;
struct ConfigData;

namespace data {
class GraphicsLoader;
}

enum class BulletGallerySceneResult : uint8_t { Running, ExitRequested };

class BulletGalleryScene {
public:
  BulletGalleryScene(const ConfigData &config, data::GraphicsLoader &graphics,
                     BulletManager &bullets, const Player &player)
      : config_(config), graphics_(graphics), bullets_(bullets),
        player_(player) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] BulletGallerySceneResult Update(INPUT_BITS input,
                                                bool should_draw);

private:
  const ConfigData &config_;
  data::GraphicsLoader &graphics_;
  BulletManager &bullets_;
  const Player &player_;
};
