/// Runtime display configuration and graphics resource recovery.

#pragma once

#include <cstdint>

#include "gfx/graphics.h"

struct GraphicsConfig;

namespace data {
class GraphicsLoader;
}

class DisplayController {
public:
  explicit DisplayController(data::GraphicsLoader &graphics)
      : graphics_(graphics) {}

  [[nodiscard]] bool Initialize(GraphicsConfig &config);
  [[nodiscard]] bool ApplyConfig(const GraphicsConfig &config);

  void SetFrameRate(uint8_t divisor);
  void SetScreenshotEffort(uint8_t effort);

private:
  [[nodiscard]] bool Apply(GRAPHICS_PARAMS requested);

  data::GraphicsLoader &graphics_;
  GRAPHICS_PARAMS params_{};
};
