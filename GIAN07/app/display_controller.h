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

  static void SetFrameRate(uint8_t divisor);
  static void SetScreenshotEffort(uint8_t effort);

private:
  [[nodiscard]] bool Apply(GraphicsParams requested);

  data::GraphicsLoader &graphics_;
  GraphicsParams params_{};
};
