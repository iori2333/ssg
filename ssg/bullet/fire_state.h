#pragma once

#include <cstdint>

#include "gfx/core/coords.h"

// ECL command accumulators. Coordinate fields retain the script format's raw
// world-unit values through WorldCoord.

struct EclBulletState {
  WorldCoord x{};
  WorldCoord y{};
  uint8_t d{};
  uint8_t dw{};
  uint8_t n{};
  uint8_t ns{};
  uint8_t v{};
  uint8_t c{};
  int8_t a{};
  int8_t vd{};
  uint8_t rep{};
  uint8_t cmd{};
  uint8_t type{};
  uint8_t option{};
};

struct EclLaserState {
  WorldCoord x{};
  WorldCoord y{};
  WorldCoord v{};
  WorldCoord w{};
  WorldCoord l{};
  WorldCoord l2{};
  uint8_t d{};
  uint8_t dw{};
  uint8_t n{};
  uint8_t c{};
  uint8_t cmd{};
  uint8_t type{};
};
