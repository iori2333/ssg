/// Debug bullet gallery scene.

#pragma once

#include <cstdint>

#include "gfx/text.h"
#include "sys/input.h"

class BulletManager;
class Player;
struct ConfigData;

namespace i18n {
class Localization;
}

namespace data {
class GraphicsLoader;
}

enum class BulletGallerySceneResult : uint8_t { Running, ExitRequested };

class BulletGalleryScene {
public:
  BulletGalleryScene(const ConfigData &config, data::GraphicsLoader &graphics,
                     BulletManager &bullets, const Player &player,
                     i18n::Localization &localization)
      : config_(config), graphics_(graphics), bullets_(bullets),
        player_(player), localization_(localization) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] BulletGallerySceneResult Update(InputBits input,
                                                bool should_draw);

private:
  const ConfigData &config_;
  data::GraphicsLoader &graphics_;
  BulletManager &bullets_;
  const Player &player_;
  i18n::Localization &localization_;
  TextRenderRectId help_text_ = 0;
};
