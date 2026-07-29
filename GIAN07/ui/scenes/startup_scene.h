/// Application startup scene.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "gfx/coords.h"

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
  struct Lens {
    [[nodiscard]] static Lens Create(uint16_t radius, uint16_t bulge);
    void Draw(WINDOW_POINT center);

    uint16_t radius;
    uint16_t diameter;
    std::vector<uint32_t> table;
    std::vector<std::byte> field_of_view;
  };

  data::GraphicsLoader &graphics_;
  std::optional<Lens> lens_;
  uint16_t timer_ = 0;
};
