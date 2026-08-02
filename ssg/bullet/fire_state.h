#pragma once

#include <cstdint>

// Raw ECL accumulators. Convert these values to typed spawn data before
// creating a bullet or laser.

struct EclBulletState {
  int x{};
  int y{};
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
  int x{};
  int y{};
  int v{};
  int w{};
  int l{};
  int l2{};
  uint8_t d{};
  uint8_t dw{};
  uint8_t n{};
  uint8_t c{};
  uint8_t cmd{};
  uint8_t type{};
};
