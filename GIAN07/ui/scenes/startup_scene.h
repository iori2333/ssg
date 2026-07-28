/// Application startup scene.

#pragma once

#include <cstdint>
#include <optional>

#include "effect/lens.h"

namespace data {
class GraphicsLoader;
}

enum class StartupSceneResult : uint8_t { Running, Complete };

class StartupScene {
public:
  explicit StartupScene(data::GraphicsLoader &graphics) : graphics_(graphics) {}

  [[nodiscard]] bool Enter();
  [[nodiscard]] StartupSceneResult Update(bool should_draw);

private:
  data::GraphicsLoader &graphics_;
  std::optional<LensInfo> lens_;
  uint16_t timer_ = 0;
};
