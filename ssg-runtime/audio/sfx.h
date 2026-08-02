/// Game sound effect identifiers and world-space pan conversion.

#pragma once

#include <cmath>
#include <cstdint>

#include "gfx/coords.h"

enum class SfxId : uint8_t {
  Kebari = 0x00,
  Tame = 0x01,
  Laser = 0x02,
  Laser2 = 0x03,
  Bomb = 0x04,
  Select = 0x05,
  Hit = 0x06,
  Cancel = 0x07,
  Warning = 0x08,
  Sblaser = 0x09,
  Buzz = 0x0a,
  Missile = 0x0b,
  Joint = 0x0c,
  Dead = 0x0d,
  Sbbomb = 0x0e,
  Bossbomb = 0x0f,
  Enemyshot = 0x10,
  Hlaser = 0x11,
  Tamefast = 0x12,
  Warp = 0x13,
};

inline constexpr int kSoundFieldCenterX = PixelToWorld(320);
inline constexpr int kSoundWorldUnitsPerDecibel = PixelToWorld(25);

[[nodiscard]] inline float SoundPanForWorldX(int x) {
  const auto relative = x - kSoundFieldCenterX;
  const auto power = relative / (kSoundWorldUnitsPerDecibel * 20.0F);
  return relative < 0 ? (std::pow(10.0F, power) - 1.0F)
                      : (1.0F - std::pow(10.0F, -power));
}
