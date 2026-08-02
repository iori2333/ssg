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

  static void SetFrameRate(int divisor);
  static void SetScreenshotEffort(int effort);

private:
  [[nodiscard]] bool Apply(GraphicsParams requested);

  data::GraphicsLoader &graphics_;
  GraphicsParams params_{};
};
