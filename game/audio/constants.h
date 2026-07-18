///
/// Shared audio types and constants — used by both game/ and GIAN07/
///
#pragma once

#include <cstdint>

#include "gfx/coords.h"

// Sound effect identifiers — shared between the audio layer and game data layer
enum class SfxId : uint8_t {
  Kebari    = 0x00, Tame      = 0x01, Laser     = 0x02, Laser2    = 0x03,
  Bomb      = 0x04, Select    = 0x05, Hit       = 0x06, Cancel    = 0x07,
  Warning   = 0x08, Sblaser   = 0x09, Buzz      = 0x0a, Missile   = 0x0b,
  Joint     = 0x0c, Dead      = 0x0d, Sbbomb    = 0x0e, Bossbomb  = 0x0f,
  Enemyshot = 0x10, Hlaser    = 0x11, Tamefast  = 0x12, Warp      = 0x13,
};

// Mapping world coordinates to a position in the stereo field.
// The algorithm from the original game:
//
// • The [x] values are world coordinates (Q26.6, 64 units per pixel)
// • Subtract the center of the screen (in world coordinates) from [x]
// • Divide the result by (a scalar) 16
// • Directly pass that result to DirectSound, which interprets it as panning
//   value with a unit of 1/100 dB

// Default X-coordinate center
constexpr int SND_X_MID = PixelToWorld(320);

constexpr int SND_X_PER_DECIBEL = PixelToWorld(25);
