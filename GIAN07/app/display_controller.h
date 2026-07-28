/// Runtime display configuration and graphics resource recovery.

#pragma once

#include <cstdint>

struct GraphicsConfig;

namespace data {
class GraphicsLoader;
}

class DisplayController {
public:
  DisplayController(GraphicsConfig &config, data::GraphicsLoader &graphics)
      : config_(config), graphics_(graphics) {}

  [[nodiscard]] bool Initialize();
  [[nodiscard]] bool ToggleFullscreen();
  [[nodiscard]] bool ToggleExclusiveFullscreen();
  [[nodiscard]] bool ToggleScalingMode();
  [[nodiscard]] bool CycleScale(int_fast8_t delta, bool include_max);
  [[nodiscard]] bool SelectApi(int8_t api);
  [[nodiscard]] bool CycleApi();

  void SetFrameRate(uint8_t divisor);
  void SetScreenshotEffort(uint8_t effort);
  void ToggleTurbo();

private:
  template <typename Modify> [[nodiscard]] bool Apply(Modify &&modify);

  GraphicsConfig &config_;
  data::GraphicsLoader &graphics_;
  uint8_t turbo_restore_divisor_ = 1;
};
