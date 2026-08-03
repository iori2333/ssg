/// Application startup scene.

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "gfx/core/coords.h"

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
    [[nodiscard]] static Lens Create(int radius, int bulge);
    void Draw(PixelPoint center);

    int radius;
    int diameter;
    std::vector<std::size_t> table;
    std::vector<uint8_t> field_of_view;
  };

  data::GraphicsLoader &graphics_;
  std::optional<Lens> lens_;
  int timer_ = 0;
};
